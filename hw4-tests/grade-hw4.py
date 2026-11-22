#!/usr/bin/env python3
# grade-hw4.py — automatic grader for hw4 (priority scheduling in xv6).
#
# Usage: run from the xv6 root directory (the directory that contains
# the Makefile):
#
#     python3 hw4-tests/grade-hw4.py
#
# The script installs three test programs (t_priofield, t_priority,
# t_sched) into user/, adds them to UPROGS if needed, builds xv6, boots
# it in QEMU and runs them. Each program covers one graded section:
#
#   t_priofield  the priority field in struct proc (default 0)
#   t_priority   the setpriority/getpriority system calls
#   t_sched      the priority scheduler
#
# Exit code: 0 = 3/3, 1 = some check failed, 2 = build/environment error.

import os
import re
import shutil
import subprocess
import sys
import time

sys.stdout.reconfigure(line_buffering=True)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TESTS = ["t_priofield", "t_priority", "t_sched"]


def die(msg, code=2):
    print(f"grade-hw4: {msg}")
    sys.exit(code)


# --------------------------------------------------------------- install --

def install_tests():
    mkpath = os.path.join(ROOT, "Makefile")
    if not os.path.exists(mkpath):
        die("Makefile not found — run this script from the xv6 directory "
            "(python3 hw4-tests/grade-hw4.py)")

    for t in TESTS:
        src = os.path.join(HERE, t + ".c")
        if not os.path.exists(src):
            die(f"missing {t}.c next to grade-hw4.py")
        shutil.copy(src, os.path.join(ROOT, "user", t + ".c"))

    with open(mkpath) as f:
        lines = f.read().split("\n")

    missing = [t for t in TESTS if not any("$U/_" + t in l for l in lines)]
    if missing:
        # Locate the UPROGS block: from "UPROGS=" up to the next
        # non-indented line (the next make rule, e.g. "fs.img:").
        i = next((i for i, l in enumerate(lines) if l.startswith("UPROGS=")),
                 None)
        if i is None:
            die("cannot find UPROGS= in Makefile")
        r = i + 1
        while r < len(lines) and (not lines[r] or lines[r][0].isspace()):
            r += 1
        block = [l for l in lines[i:r] if l.rstrip() != "\\"]
        entries = [k for k, l in enumerate(block)
                   if l.lstrip().startswith("$U/")]
        if not entries:
            die("cannot find UPROGS entries in Makefile")
        last = entries[-1]
        # Normalize every entry to end with exactly one backslash, so
        # the new lines can be appended after the last real entry.
        for k in entries:
            block[k] = block[k].rstrip().rstrip("\\").rstrip() + "\\"
        for k, t in enumerate(missing):
            entry = "\t$U/_" + t
            if k != len(missing) - 1:
                entry += "\\"
            block.insert(last + 1 + k, entry)
        lines[i:r] = block
        with open(mkpath, "w") as f:
            f.write("\n".join(lines))
        print(f"grade-hw4: added {', '.join(missing)} to Makefile UPROGS")

    # Not graded here, but part of the deliverable: warn if the student's
    # demo programs are missing.
    for prog in ("priority_test", "setpriority_test"):
        if not os.path.exists(os.path.join(ROOT, "user", prog + ".c")):
            print(f"grade-hw4: warning: user/{prog}.c not found")


# ----------------------------------------------------------------- build --

def build():
    print("grade-hw4: building xv6 ...")
    r = subprocess.run(["make", "kernel/kernel", "fs.img"], cwd=ROOT,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True)
    if r.returncode != 0:
        print(r.stdout[-5000:])
        die("build failed — fix compile errors first", 2)


# ------------------------------------------------------------------ QEMU --

class Qemu:
    def __init__(self):
        # A single CPU makes the priority-scheduler check deterministic:
        # with the default 3 CPUs several children could run at the same
        # time and finish in an arbitrary order.
        self.proc = subprocess.Popen(["make", "qemu", "CPUS=1"], cwd=ROOT,
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)
        os.set_blocking(self.proc.stdout.fileno(), False)
        self.out = bytearray()

    def drain(self, secs=0.3):
        end = time.time() + secs
        while time.time() < end:
            try:
                b = os.read(self.proc.stdout.fileno(), 4096)
                if b:
                    self.out.extend(b)
                else:
                    time.sleep(0.05)
            except (BlockingIOError, ValueError):
                time.sleep(0.05)

    def waitfor(self, pat, timeout):
        end = time.time() + timeout
        while time.time() < end:
            self.drain(0.3)
            if re.search(pat, self.text()):
                return True
        return False

    def send(self, cmd):
        self.proc.stdin.write(cmd.encode())
        self.proc.stdin.flush()

    def text(self):
        return self.out.decode("utf-8", "replace")

    def stop(self):
        self.proc.terminate()
        try:
            self.proc.wait(5)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def run_test(q, name, timeout):
    """Run one test program; return (ok, reason)."""
    q.out = bytearray()  # only look at this command's output
    q.send(name + "\n")
    pat = rf"{name}: (OK|FAILED)|panic|usertrap|scause|unexpected trap"
    if not q.waitfor(pat, timeout):
        text = q.text()
        if re.search(r"exec .* failed|not found", text):
            return False, "program missing (not in UPROGS?)"
        return False, "timed out — the scheduler may never run this program"
    q.drain(0.3)  # let the rest of the line arrive
    if f"{name}: OK" in q.text():
        return True, ""
    m = re.search(rf"{name}: FAILED: (.*)", q.text())
    if m:
        return False, m.group(1).strip()
    return False, "kernel trapped:\n" + q.text()[-1500:]


# ------------------------------------------------------------------ main --

CHECKS = [
    ("t_priofield", "priority field", 60),
    ("t_priority", "system calls", 60),
    ("t_sched", "scheduler", 180),
]


def main():
    install_tests()
    build()

    results = []
    q = Qemu()
    try:
        if not q.waitfor(r"\$ ", 90):
            die("xv6 did not reach the shell prompt", 2)
        print("grade-hw4: xv6 booted, running tests ...\n")
        for name, label, timeout in CHECKS:
            ok, reason = run_test(q, name, timeout)
            results.append((name, label, ok, reason))
            q.waitfor(r"\$ ", 5)  # let the shell prompt return
    finally:
        q.stop()

    print()
    score = 0
    for name, label, ok, reason in results:
        if ok:
            score += 1
            print(f"[OK]   {name:<12} ({label})")
        else:
            print(f"[FAIL] {name:<12} ({label})  {reason}")
    print(f"\nscore: {score}/3")
    sys.exit(0 if score == 3 else 1)


if __name__ == "__main__":
    main()

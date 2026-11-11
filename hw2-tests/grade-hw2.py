#!/usr/bin/env python3
# grade-hw2.py — automatic grader for hw2 (processes).
#
# Usage: run from the xv6 root directory (the directory that contains
# the Makefile):
#
#     python3 hw2-tests/grade-hw2.py
#
# The script installs the test programs (t_pinfo, t_killtree) into
# user/ (adding them to the Makefile if needed), builds xv6, boots it
# in QEMU, runs the homework's programs and the tests, and prints a
# score table. Exit code 0 means 5/5.

import os
import re
import shutil
import subprocess
import sys
import time

sys.stdout.reconfigure(line_buffering=True)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TESTS = ["t_pinfo", "t_killtree"]


def die(msg, code=2):
    print(f"grade-hw2: {msg}")
    sys.exit(code)


def install_tests():
    if not os.path.exists(os.path.join(ROOT, "Makefile")):
        die("Makefile not found — run this script from the xv6 directory "
            "(python3 hw2-tests/grade-hw2.py)")
    for t in TESTS:
        shutil.copy(os.path.join(HERE, t + ".c"),
                    os.path.join(ROOT, "user", t + ".c"))

    mkpath = os.path.join(ROOT, "Makefile")
    with open(mkpath) as f:
        lines = f.read().split("\n")
    missing = [t for t in TESTS if not any("$U/_" + t in l for l in lines)]
    if missing:
        # Locate the UPROGS block: from "UPROGS=" up to the next
        # non-indented line (the next make rule, e.g. "fs.img:").
        i = next(i for i, l in enumerate(lines) if l.startswith("UPROGS="))
        r = i + 1
        while r < len(lines) and (not lines[r] or lines[r][0].isspace()):
            r += 1
        block = [l for l in lines[i:r] if l.rstrip() != "\\"]
        entries = [k for k, l in enumerate(block)
                   if l.lstrip().startswith("$U/")]
        if not entries:
            die("cannot find UPROGS entries in Makefile", 2)
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
        print(f"grade-hw2: added {', '.join(missing)} to Makefile UPROGS")


def build():
    print("grade-hw2: building xv6 ...")
    r = subprocess.run(["make", "kernel/kernel", "fs.img"], cwd=ROOT,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True)
    if r.returncode != 0:
        print(r.stdout[-4000:])
        die("build failed — fix compile errors first", 2)


class Qemu:
    def __init__(self):
        self.proc = subprocess.Popen(["make", "qemu"], cwd=ROOT,
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


def main():
    install_tests()
    build()

    checks = [
        # name, command, pattern that must appear, timeout
        ("pingpong", "pingpong",
         r"pingpong: \d+ exchanges in \d+ ticks\npingpong: \d+ exchanges/sec",
         60),
        ("race", "race", None, 60),  # special: reap order
        ("t_pinfo", "t_pinfo", r"t_pinfo: OK", 30),
        ("t_killtree", "t_killtree", r"t_killtree: OK", 40),
        ("ps", "ps",
         r"PID\tPPID\tAGE\tSTATE\tNAME[\s\S]*?\n1\t0\t\d+\t", 30),
    ]

    q = Qemu()
    results = []
    try:
        if not q.waitfor(r"\$", 60):
            die("xv6 did not reach the shell prompt", 2)
        print("grade-hw2: xv6 booted, running tests ...\n")

        for name, cmd, pat, timeout in checks:
            q.out = bytearray()  # only look at output of this command
            q.send(cmd + "\n")
            if name == "race":
                ok = q.waitfor(r"race: done", timeout)
                statuses = re.findall(
                    r"reaped child pid \d+ with status (\d+)", q.text())
                if ok and statuses == ["5", "4", "3", "2", "1"]:
                    results.append((name, True, ""))
                else:
                    results.append((name, False,
                                    f"reap order {statuses} (want 5,4,3,2,1)"))
                continue

            ok = q.waitfor(pat, timeout)
            reason = ""
            if not ok:
                if re.search(r"not found|exec .* failed", q.text()):
                    reason = "program missing (not in UPROGS?)"
                else:
                    reason = "expected output not seen"
            results.append((name, ok, reason))
    finally:
        q.stop()

    print()
    score = 0
    for name, ok, reason in results:
        if ok:
            score += 1
            print(f"[OK]   {name}")
        else:
            print(f"[FAIL] {name}  {reason}")
    print(f"\nscore: {score}/5")
    sys.exit(0 if score == 5 else 1)


if __name__ == "__main__":
    main()

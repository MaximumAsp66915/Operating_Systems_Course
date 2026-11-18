#!/usr/bin/env python3
# grade-hw3.py — automatic grader for hw3 (user-level threads in xv6).
#
# Usage: run from the xv6 root directory:
#
#     python3 hw3-tests/grade-hw3.py
#
# It patches the Makefile so user/_uthread is built (UPROGS entry + the
# rules needed to assemble uthread_switch.S), builds xv6, boots it in QEMU
# and runs uthread, checking the exact 13-line round-robin output.
#
# Exit code: 0 = 1/1, 1 = check failed, 2 = build/environment error.

import os, re, subprocess, sys, time

sys.stdout.reconfigure(line_buffering=True)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def die(msg, code=2):
    print(f"grade-hw3: {msg}")
    sys.exit(code)


# ---------------------------------------------------------------- Makefile --

def find_block(lines, start_marker):
    """Return (i, r): the range [i, r) of the block after start_marker
    (e.g. 'UPROGS='). The block ends at the first line that is neither
    blank nor indented."""
    i = next((i for i, l in enumerate(lines) if l.startswith(start_marker)), None)
    if i is None:
        return None
    r = i + 1
    while r < len(lines) and (not lines[r].strip() or lines[r][0].isspace()):
        r += 1
    return i, r


def patch_uprogs(lines):
    if any("$U/_uthread" in l for l in lines):
        return False
    blk = find_block(lines, "UPROGS=")
    if blk is None:
        die("cannot find UPROGS= in Makefile")
    i, r = blk
    block = [l for l in lines[i:r] if l.rstrip() != "\\"]
    entries = [k for k, l in enumerate(block) if l.lstrip().startswith("$U/")]
    if not entries:
        die("no $U/ entries found in UPROGS")
    for k in entries:
        block[k] = block[k].rstrip().rstrip("\\").rstrip() + "\\"
    block.insert(entries[-1] + 1, "\t$U/_uthread\\")
    lines[i:r] = block
    return True


def _rule_end(lines, idx):
    """Index just past the recipe (tab-indented lines) of the rule at idx."""
    i = idx + 1
    while i < len(lines) and (lines[i].startswith("\t") or not lines[i].strip()):
        i += 1
    return i


def patch_s_rules(lines):
    """Vanilla xv6 has no rule to assemble a .S file for a user program and
    links user programs with the '$U/_%: %.o' pattern rule. Add both, since
    uthread is built from uthread.c + uthread_switch.S."""
    changed = False

    if not any("$U/%.o: $U/%.S" in l for l in lines):
        anchor = next((i for i, l in enumerate(lines)
                       if l.startswith("$U/%.o:")), None)
        if anchor is None:
            anchor = next((i for i, l in enumerate(lines)
                           if l.startswith("$K/%.o: $K/%.S")), None)
        if anchor is None:
            anchor = next((i for i, l in enumerate(lines)
                           if l.startswith(".PRECIOUS:")), None)
        pos = _rule_end(lines, anchor) if anchor is not None else len(lines)
        lines[pos:pos] = ["$U/%.o: $U/%.S",
                          "\t$(CC) $(CFLAGS) -c -o $@ $<", ""]
        changed = True

    if not any("_uthread: $U/uthread.o" in l for l in lines):
        anchor = next((i for i, l in enumerate(lines)
                       if l.startswith("$U/_%: %.o")), None)
        if anchor is None:
            anchor = next((i for i, l in enumerate(lines)
                           if l.startswith("_%: %.o")), None)
        if anchor is None:
            die("cannot find the user-program link rule in Makefile")
        pos = _rule_end(lines, anchor)
        lines[pos:pos] = [
            "$U/_uthread: $U/uthread.o $U/uthread_switch.o $(ULIB)",
            "\t$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $< "
            "$U/uthread_switch.o $(ULIB)",
            "\t$(OBJDUMP) -S $@ > $U/uthread.asm",
            "\t$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; "
            "/^$$/d' > $U/uthread.sym",
            ""]
        changed = True

    return changed


def patch_makefile():
    mkpath = os.path.join(ROOT, "Makefile")
    if not os.path.exists(mkpath):
        die("Makefile not found — run this script from the xv6 directory")
    with open(mkpath) as f:
        text = f.read()
    lines = text.split("\n")
    if patch_uprogs(lines):
        print("grade-hw3: added $U/_uthread to UPROGS")
    if patch_s_rules(lines):
        print("grade-hw3: added .S build rules")
    new = "\n".join(lines)
    if new != text:
        with open(mkpath, "w") as f:
            f.write(new)


# ------------------------------------------------------------------- build --

def check_sources():
    missing = [f for f in ("uthread.c", "uthread_switch.S")
               if not os.path.exists(os.path.join(ROOT, "user", f))]
    if missing:
        die("missing file(s) in user/: " + ", ".join(missing))


def build():
    print("grade-hw3: building xv6 ...")
    r = subprocess.run(["make", "kernel/kernel", "fs.img"], cwd=ROOT,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True)
    if r.returncode != 0:
        print(r.stdout[-5000:])
        die("build failed — fix compile errors first", 2)


# -------------------------------------------------------------------- QEMU --

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


EXPECTED = [
    "uthread: thread 0 tick 0", "uthread: thread 1 tick 0",
    "uthread: thread 2 tick 0", "uthread: thread 0 tick 1",
    "uthread: thread 1 tick 1", "uthread: thread 2 tick 1",
    "uthread: thread 0 tick 2", "uthread: thread 1 tick 2",
    "uthread: thread 2 tick 2", "uthread: thread 0 finished",
    "uthread: thread 1 finished", "uthread: thread 2 finished",
    "uthread: done",
]


def ordered_lines(text, lines):
    idx = 0
    for line in lines:
        m = re.search(re.escape(line), text[idx:])
        if not m:
            return False, f"missing line: {line}"
        idx += m.end()
    return True, ""


def check_uthread():
    q = Qemu()
    try:
        if not q.waitfor(r"\$ ", 90):
            die("xv6 did not reach the shell prompt", 2)
        q.out = bytearray()
        q.send("uthread\n")
        if not q.waitfor(r"uthread: done|panic|usertrap|scause", 45):
            return False, "no uthread output:\n" + q.text()[-2000:]
        if "uthread: done" not in q.text():
            return False, "kernel trapped before finishing:\n" + q.text()[-1500:]
        return ordered_lines(q.text(), EXPECTED)
    finally:
        q.stop()


def main():
    check_sources()
    patch_makefile()
    build()

    ok, reason = check_uthread()
    print()
    print(("[OK]  " if ok else "[FAIL]") + " uthread" +
          (f"  {reason}" if reason else ""))
    print(f"\nscore: {1 if ok else 0}/1")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()

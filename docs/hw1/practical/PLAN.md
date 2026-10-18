# HW1 – Practical part: English translation and step-by-step plan

Translation of the practical section (pages 6-10) of `../pre_provided/1405_OS_HW1.pdf`,
followed by a plan. No written report is required for the practical part, but
**every step needs a screenshot or recording, committed and pushed**.

General rules from page 1: everything is submitted as `HW1_<id1>_<id2>.zip` containing
the practical outputs and the theory PDF. Submission must be in Persian only for the
theory part (English = zero). Collect screenshots, videos and sources in one folder.

---

## 1. Translation

### Practical Exercise 1 – xv6 (the course is based on MIT's xv6; links and videos are on the CW page)

1. **Run xv6.** Using the tutorial video on CW, run xv6 and take screenshots of the steps.
   Debian 13 or Ubuntu 24 is recommended. If you don't use Linux, install a Debian/Ubuntu VM
   (VirtualBox guides are linked in the handout). First install the build prerequisites
   (compiler tools and QEMU) as in the video. The minimum QEMU version for xv6 is 7.2.
2. **Hello World.** Add a user program that prints `Hello World!` to the terminal. Make the
   needed changes in the OS and add the program to the Makefile. Run it and screenshot the output.
3. **Custom system call `power`.** It takes two integers `a` and `b` and returns `a^b` to user
   space. Then write a user program that calls it and prints the result. Expected output:
   ```
   $ power
   2^10 = 1024
   3^4 = 81
   ```
   Implement both sides:
   - **Kernel side:** define the syscall number, implement it in the kernel, register it in the syscall table.
   - **User side:** define the new syscall in user space and write the program that calls it.

   Note: assume `a` and `b` are non-negative and the result fits in a 32-bit integer.
   Compile, run xv6 several times with different inputs, and screenshot.

**Deliverables:** (1) a zip with the Hello World source and Makefile diff, the power source and all
changed files for the syscall, and screenshots of running xv6 and the programs. (2) A video of at most
5-6 minutes showing: running xv6 and a few simple commands; running Hello World; running `power`
(at least twice with different inputs); a short explanation of the call path from user space into the
kernel and back. Briefly explain the steps; no line-by-line explanation needed.

### Practical Exercise 2 – Docker and comparing Bare Metal / VM / Docker

You get to know Docker installation, pulling images, creating containers and their lifecycle.

1. **Install (10 pts).** Install Docker Engine on a Linux host. Record the Docker version and the
   output of a successful hello-world run:
   ```
   docker --version
   docker info
   docker run --rm hello-world
   ```
2. **Image and container (15 pts).** Pull `ubuntu:24.04`, list images, and run an interactive
   container named `os`. Inside it view the kernel version and distribution info, create the file
   `/tmp/os.txt`, and exit:
   ```
   docker pull ubuntu:24.04
   docker images
   docker run -it --name os ubuntu:24.04 bash
   uname -r
   cat /etc/os-release
   touch /tmp/os.txt
   exit
   ```
3. **Container management (10 pts).** Check the state of `os` with `docker ps -a`. Start it again with
   `docker start -ai os` and check whether the file survived. Then run a background container, and
   stop / restart / stop / remove it in order. At the end delete `os` as well:
   ```
   docker run -d --name os-bg ubuntu:24.04 sleep 600
   docker ps
   docker stop os-bg
   docker start os-bg
   docker stop os-bg
   docker rm os-bg
   docker rm os
   ```
   **Question (5 pts):** What is the difference between an Image and a Container? Why does exiting the
   shell stop the container, yet the created file remains after a restart? (at most five lines)

**Comparing Bare Metal, VM and Docker.** Compare the structure and performance of the three
environments. You need a Linux host with a **direct (bare-metal) install**; run Docker on this same host
and also create a Linux VM on it. Install the tool `sysbench` in all three. *Docker Desktop on Windows or Mac is not an
acceptable substitute for bare metal.*

Run these three workloads in each environment and compare:
```
# cpu-intensive
sysbench cpu --threads=4 --cpu-max-prime=2000000 --percentile=99 run
# memory-intensive
sysbench memory --memory-total-size=20G --memory-access-mode=rnd run
# io-intensive
sysbench fileio --file-total-size=4G prepare
sysbench fileio --file-total-size=4G --file-test-mode=seqwr --percentile=99 run
sysbench fileio --file-total-size=4G cleanup
```
**Question (10 pts):** Describe what each workload does in one line. Before running, state your predicted
relative performance of the three environments for each workload, with a brief reason.

Run each workload in all three environments and make **one chart per workload (three charts total)**
with these metrics:
- cpu-intensive: P99 latency, average latency
- memory-intensive: total time, throughput
- io-intensive: P99 latency, average latency, write throughput

Note: for a fair comparison use the same sysbench version and parameters; the number of vCPUs and the
VM's memory must be equal; state them in the report along with the file access method in Docker
(bind mount of the filesystem or a file). Repeat each test at least five times and plot the mean.
If a workload does not generate enough events for measurement, reduce `cpu-max-prime` **equally in all three
environments** and record the change. For metrics with different units use separate subplots in the chart.

**Question (10 pts):** Did the results match your prediction? Explain the observed differences using how
the environments execute processes, access the kernel, and the virtualization overhead of each.

**What to submit:** one PDF containing the important commands and their outputs, the answers, **three
charts**, the specifications of the environments, and a short description of the steps. Marks for the
comparison part: runs and charts (40), prediction and workload explanation (10), result analysis (10).

### Practical Exercise 3 – Build a Linux kernel, add a module and a system call

Goal: practical familiarity with compiling Linux and adding custom modules and system calls for
development and debugging. Briefly, you must: download and configure the kernel source; enable the
debug-related modules in the configuration; compile the new kernel; write and compile a simple
kernel module (`.ko`); add and remove the compiled module in the kernel.

**Prerequisites:** a virtual or physical machine running a Linux distro (preferably Debian 12);
**the username must contain your names**: `osvm@lastname1-lastname2$`; at least 15 GB free disk.
Tools after installing the OS:
```
sudo apt update
sudo apt install -y build-essential bc bison flex libssl-dev libelf-dev \
    libncurses-dev dwarves fakeroot cpio rsync kmod
```

**Steps**
1. *Get the source:* download kernel version **6.1.189**.
2. *Configure:* (a) copy your current config: `cp /boot/config-$(uname -r) .config`
   (b) run `make menuconfig` (c) enable: Kernel Hacking → Kernel debugging; Kernel Hacking → Memory
   debugging (options like KASAN); Enable loadable module support (if not enabled).
   (d) Tip: `make localmodconfig` removes unneeded drivers and speeds up the build.
3. *Compile:* (a) `make -j$(nproc)`, `sudo make modules_install`, `sudo make install` (can take long)
   (b) update GRUB, reboot, and choose the new kernel (c) verify with `uname -r`.
4. *Develop and test a simple module:* (a) a kernel module that prints a message to `/var/log/kern.log`
   when loaded and unloaded (b) a simple Makefile for it (c) compile with make (d) commands that load
   and unload it: find it in the kernel with the commands, and load/remove it (e) view the log with
   `sudo dmesg | tail` or `/var/log/kern.log`.
5. *Add and use a system call:* (a) add a syscall `hello_syscall` to the kernel. It takes no input,
   returns 42, and prints `hello_syscall: Hello from the kernel!` to the kernel log. (b) implement it in
   a suitable place and register it in the x86-64 syscall table (c) recompile, install, and boot the new
   kernel (d) write a simple C program that calls the new syscall from user space and prints its
   return value, e.g.:
   ```
   Calling hello_syscall...
   hello_syscall returned: 42
   ```
   (e) compile and run it; check the message in `sudo dmesg | tail` (f) in your video explain how
   executing the syscall moves the program from user space to kernel space and back.

**Deliverables:** (1) a zip with the final kernel `.config`, the module source `.c`, the module Makefile,
the user program source, and the files modified for the syscall. (2) A 5-6 minute video showing: the
compiled kernel running; compiling, loading and unloading the module and its log messages; the syscall
changes; running the user program and a successful call; the return value and the kernel-log message.
Explain briefly, no line-by-line.

---

## 2. Plan

Work order: **Part 1 (xv6) → Part 2 (Docker/sysbench) → Part 3 (Linux kernel, slowest)**.
Start the kernel compile early because it takes a long time.

**Evidence rule:** after every step take a screenshot (or note the video timestamp), save it in
`docs/hw1/practical/evidence/<part>/NN-description.png`, then
```
git add -A && git commit -m "hw1: <step description>" && git push
```
xv6 code commits go on branch `hw1`; screenshots/scripts/docs go on `main` (see the root README).
Record one 5-6 min video for part 1 and one for part 3; keep videos out of git (they are ignored) and
store them in `recordings/` locally.

### Part 1 – xv6 (≈30 min)  — **already implemented on branch `hw1`**
The code is on `hw1` (commits `hw1: add hello user program`, `hw1: add power system call and user program`).
You only need to run and capture it:

| # | Step | Command / action | Evidence |
|---|------|------------------|----------|
| 1.1 | Install tools | `sudo apt install build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu` ; `qemu-system-riscv64 --version` (≥7.2) | screenshot of versions |
| 1.2 | Boot xv6 | `git switch hw1 && make qemu`, run `ls`, `cat README`; quit with Ctrl-a x | screenshot |
| 1.3 | Hello World | `hello` | screenshot of `Hello World!` |
| 1.4 | power syscall | `power 2 10`, `power 3 4`, plus one more input (e.g. `power 5 0`) | screenshot |
| 1.5 | Video | show 1.2-1.4 and explain the path below | `recordings/` |
| 1.6 | Push | `git push origin hw1` | — |

What changed (explain this in the video): `user/hello.c`, `user/power.c` + `UPROGS` in `Makefile`;
`kernel/syscall.h` (`SYS_power 22`), `kernel/syscall.c` (extern + table entry), `kernel/sysproc.c`
(`sys_power`), `user/usys.pl` (stub), `user/user.h` (prototype).
Call path: `power()` stub loads `SYS_power` into `a7` and runs `ecall` → trap into `usertrap()` →
`syscall()` reads `a7`, indexes the `syscalls[]` table, calls `sys_power()`, which reads args with
`argint(0)`/`argint(1)`; the return value is written to the trapframe's `a0`, `usertrapret()` runs `sret`
back to user mode, and the stub returns it.

### Part 2 – Docker and benchmarks (≈3 h, mostly waiting)
Host must be **bare-metal Linux** (it has your 15 GB free).

1. Install Docker Engine (`https://docs.docker.com/engine/install/`), then run the 3 commands of task 2.1 → screenshots.
2. Tasks 2.2 and 2.3 exactly as in the translation → screenshots, and write the 5-line answer.
3. Create the VM (VirtualBox or QEMU/KVM) on the same host, **same vCPU count and RAM** for all runs; note them
   (`nproc`, `free -h`).
4. Install `sysbench` (same version in all three: check `sysbench --version`).
5. Write your **predictions** first (commit them: `evidence/2-docker/predictions.md`) before running anything.
6. Run `docker/bench.sh` in each environment (5 repeats):
   ```
   # host
   ./bench.sh host
   # VM
   ./bench.sh vm
   # docker: bind mount for the file-io directory, same sysbench version inside
   docker run --rm -it -v "$PWD":/work -w /work -e BENCH_DIR=/work/io ubuntu:24.04 bash
   #   apt update && apt install -y sysbench && mkdir -p io && ./bench.sh docker
   ```
   Copy the three CSVs together and run `python3 plot.py results/host.csv results/vm.csv results/docker.csv`
   → `cpu.png`, `memory.png`, `io.png`. (The scripts were not run on this machine because it has no
   sysbench; test with `./bench.sh host 1` first and fix the awk patterns if sysbench's output differs.)
7. Write the PDF (commands, outputs, answers, 3 charts, environment specs, analysis) — LaTeX is fine.

### Part 3 – Linux kernel (≈half a day, mostly compiling)
Use a Debian 12 VM named so that the prompt is `osvm@<lastname1>-<lastname2>` and has ≥15 GB free
(give it 8 GB+ and all cores for the compile).

1. Tools: the `apt install` line from the translation.
2. Get source: `wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.1.189.tar.xz && tar xf linux-6.1.189.tar.xz`.
3. Config: `cd linux-6.1.189; cp /boot/config-$(uname -r) .config; make localmodconfig` (accept defaults),
   then `make menuconfig` and enable the debugging options of step 2c.
   **Debian-specific fix** (otherwise the build fails): 
   ```
   scripts/config --disable SYSTEM_TRUSTED_KEYS
   scripts/config --disable SYSTEM_REVOCATION_KEYS
   scripts/config --disable DEBUG_INFO_BTF
   ```
4. Add the syscall **before** the first build to save one long compile: apply the two edits in
   `linux/syscall/hello_syscall.patch` (first check `tail arch/x86/entry/syscalls/syscall_64.tbl` – 451 should be unused
   in 6.1; if not, use the next free number and change it in the test program).
5. Build: `make -j$(nproc)` → `sudo make modules_install` → `sudo make install` → `sudo update-grub` → reboot
   (pick the new kernel in GRUB) → `uname -r` (screenshot: it must say 6.1.189 …).
6. Module: copy `linux/hello_mod/` into the VM, then
   `make`, `sudo insmod hello_mod.ko`, `lsmod | grep hello_mod`, `sudo rmmod hello_mod`, `sudo dmesg | tail`.
7. Syscall test: `gcc test_hello_syscall.c -o test_hello_syscall && ./test_hello_syscall && sudo dmesg | tail`.
8. Record the 5-6 min video (kernel version, module load/unload + dmesg, syscall edits, test program + dmesg line).
   Explain: `syscall` instruction → CPU switches to ring 0 and jumps to `entry_SYSCALL_64` → saves registers →
   indexes `sys_call_table` with `rax` → runs `__x64_sys_hello_syscall` → return value in `rax` → `sysret` back to ring 3.
9. Collect for submission: `.config`, `hello_mod.c`, module `Makefile`, `test_hello_syscall.c`, the diff
   of the kernel tree (`diff -u` of `syscall_64.tbl` and `kernel/sys.c`).

### Final packaging
`HW1_402101906_402110949.zip` = theory PDF + `evidence/`, source/patch files above, the Part-2 PDF, and the videos.

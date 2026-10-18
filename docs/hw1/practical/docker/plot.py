#!/usr/bin/env python3
"""Usage: python3 plot.py results/host.csv results/vm.csv results/docker.csv
Makes cpu.png, memory.png, io.png (one figure per workload, 2 subplots each,
bars = mean over runs, error bars = standard deviation)."""
import csv, sys, statistics as st
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

PLOTS = {
    "cpu":    [("cpu_p99_ms", "P99 latency (ms)"), ("cpu_avg_ms", "Average latency (ms)")],
    "memory": [("mem_total_time_s", "Total time (s)"), ("mem_throughput_mibs", "Throughput (MiB/s)")],
    "io":     [("io_p99_ms", "P99 latency (ms)"), ("io_avg_ms", "Average latency (ms)"),
               ("io_write_mibs", "Write throughput (MiB/s)")],
}

data = {}
for path in sys.argv[1:]:
    rows = list(csv.DictReader(open(path)))
    data[rows[0]["env"]] = rows

for name, metrics in PLOTS.items():
    fig, axes = plt.subplots(1, len(metrics), figsize=(4.5 * len(metrics), 4))
    for ax, (col, title) in zip(axes, metrics):
        envs = list(data)
        vals = [[float(r[col]) for r in data[e]] for e in envs]
        ax.bar(envs, [st.mean(v) for v in vals],
               yerr=[st.pstdev(v) for v in vals], capsize=4)
        ax.set_title(title)
    fig.suptitle(f"{name}-intensive")
    fig.tight_layout()
    fig.savefig(f"{name}.png", dpi=150)
    print("wrote", f"{name}.png")

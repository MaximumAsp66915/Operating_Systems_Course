#!/usr/bin/env bash
# Usage: ./bench.sh <label> [repeats]      label = host | vm | docker
# Run it in each environment with the SAME sysbench version and the SAME vCPU/RAM.
# For docker the file-io directory must be a bind mount (see PLAN.md).
# Writes results/<label>.csv with one row per run.
set -euo pipefail
label=${1:?label (host|vm|docker)}
runs=${2:-5}
workdir=${BENCH_DIR:-$PWD}          # where fileio test files are created
mkdir -p results
out=results/$label.csv
echo "env,run,cpu_p99_ms,cpu_avg_ms,mem_total_time_s,mem_throughput_mibs,io_p99_ms,io_avg_ms,io_write_mibs" > "$out"
sysbench --version | tee results/$label.version

for i in $(seq 1 "$runs"); do
  cpu=$(sysbench cpu --threads=4 --cpu-max-prime=2000000 --percentile=99 run)
  mem=$(sysbench memory --memory-total-size=20G --memory-access-mode=rnd run)
  (cd "$workdir" && sysbench fileio --file-total-size=4G prepare >/dev/null)
  io=$(cd "$workdir" && sysbench fileio --file-total-size=4G --file-test-mode=seqwr --percentile=99 run)
  (cd "$workdir" && sysbench fileio --file-total-size=4G cleanup >/dev/null)

  cpu_p99=$(awk '/99th percentile/ {print $3}' <<<"$cpu")
  cpu_avg=$(awk '/^ *avg:/ {print $2}' <<<"$cpu")
  mem_t=$(awk '/total time:/ {gsub("s","",$3); print $3}' <<<"$mem")
  mem_tp=$(awk '/MiB transferred/ {gsub("\\(","",$4); print $4}' <<<"$mem")
  io_p99=$(awk '/99th percentile/ {print $3}' <<<"$io")
  io_avg=$(awk '/^ *avg:/ {print $2}' <<<"$io")
  io_w=$(awk '/written, MiB\/s:/ {print $3}' <<<"$io")
  echo "$label,$i,$cpu_p99,$cpu_avg,$mem_t,$mem_tp,$io_p99,$io_avg,$io_w" | tee -a "$out"
done

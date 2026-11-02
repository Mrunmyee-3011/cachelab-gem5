#!/usr/bin/env bash
# Runs the booking workload on GEM5 for 2/4/8 cores x {shared, padded}, then collects the stats files.
# Usage:  GEM5=/path/to/gem5/build/X86/gem5.opt bash run_all.sh [iters]
set -euo pipefail
GEM5="${GEM5:?set GEM5 to the path of your gem5.opt binary}"
ITERS="${1:-1000}"
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/results"
mkdir -p "$OUT/stats"

gcc -O2 -static -pthread -o "$HERE/booking" "$HERE/booking.c"
"$HERE/booking" 4 1000 0   # native sanity check (must print OK)

for cores in 2 4 8; do
  for mode in 0 1; do
    name=$([ "$mode" = 0 ] && echo shared || echo padded)
    run="${name}_${cores}c"
    echo "=== $run ==="
    "$GEM5" -d "$OUT/$run" "$HERE/se_mesi.py" --cores "$cores" --binary "$HERE/booking" --iters "$ITERS" --mode "$mode"
    cp "$OUT/$run/stats.txt" "$OUT/stats/${run}.stats.txt"
  done
done
echo "Done. Import the files in $OUT/stats into CacheLab > GEM5 / Multi2Sim."

#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
make debug
baseline=$(find /proc/$$/fd -maxdepth 1 -type l | wc -l)
for i in $(seq 1 100); do
  ./progvoraz --input tests/sample_batch.csv --output "/tmp/progvoraz_fd_$i.csv" --log "/tmp/progvoraz_fd_$i.log" >/dev/null
  printf 'mode,currency,amount\na,EUR,127\nb,EUR,127\n' | ./progvoraz --stream >/dev/null
done
after=$(find /proc/$$/fd -maxdepth 1 -type l | wc -l)
if [ "$after" -gt $((baseline + 3)) ]; then
  echo "FD leak suspected: baseline=$baseline after=$after" >&2
  exit 1
fi
if command -v strace >/dev/null 2>&1; then
  strace -ff -e trace=%file,%desc -o /tmp/progvoraz_strace ./progvoraz --input tests/sample_batch.csv --output /tmp/progvoraz_strace.csv >/dev/null
fi
echo "FD stable: baseline=$baseline after=$after"

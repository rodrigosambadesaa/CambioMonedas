#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
make debug
rss_max=0
for i in $(seq 1 200); do
  input="/tmp/progvoraz_stress_$i.csv"
  { echo 'mode,currency,amount,range'; echo "a,EUR,$((i % 500 + 1)),"; echo "b,EUR,$((i % 200 + 1)),"; echo 'a,NOPE,10,'; echo 'a,EUR,,'; } > "$input"
  if command -v /usr/bin/time >/dev/null 2>&1; then
    /usr/bin/time -f '%M' -o /tmp/progvoraz_rss ./progvoraz --input "$input" --output "/tmp/progvoraz_stress_$i.out" >/dev/null
    rss=$(cat /tmp/progvoraz_rss)
    [ "$rss" -gt "$rss_max" ] && rss_max=$rss
  else
    ./progvoraz --input "$input" --output "/tmp/progvoraz_stress_$i.out" >/dev/null
  fi
done
if [ "$rss_max" -gt 0 ]; then
  echo "Stress OK; peak RSS ${rss_max} KB"
else
  echo "Stress OK; /usr/bin/time unavailable, RSS sampling skipped"
fi

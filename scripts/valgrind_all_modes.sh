#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
if ! command -v valgrind >/dev/null 2>&1; then
  echo "valgrind no está instalado; omitiendo comprobación local (docker/Dockerfile.debug sí lo instala)." >&2
  exit 0
fi
make debug
VG=(valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite,possible --error-exitcode=1)
"${VG[@]}" ./.dist/test_progvoraz
"${VG[@]}" ./progvoraz --help >/dev/null
"${VG[@]}" ./progvoraz --version >/dev/null
"${VG[@]}" ./progvoraz --input tests/sample_batch.csv --output /tmp/progvoraz_vg_out.csv --log /tmp/progvoraz_vg.log
printf 'mode,currency,amount,range\na,EUR,127,\nb,EUR,127,\nc,EUR,127,1-20\na,NOPE,10,\na,EUR,,\n' | "${VG[@]}" ./progvoraz --stream >/tmp/progvoraz_vg_stream.csv
"${VG[@]}" ./progvoraz --export-stock-json /tmp/progvoraz_stock.json >/dev/null
"${VG[@]}" ./progvoraz --export-report /tmp/progvoraz_report.txt >/dev/null

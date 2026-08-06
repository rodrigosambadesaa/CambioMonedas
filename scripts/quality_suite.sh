#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

make clean
make release
make gui

./docker/run_all_modes_tests.sh
make memory-safety

echo "Suite Docker consolidada completada correctamente."

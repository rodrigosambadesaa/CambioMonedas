#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1"
make clean
make test-sanitize
make valgrind
make fd-check
make stress-test

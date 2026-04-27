#!/usr/bin/env bash
set -euo pipefail

mode="${1:-console}"
shift || true

case "$mode" in
  console)
    exec /app/progvoraz "$@"
    ;;
  gui)
    exec /app/progvoraz_gui "$@"
    ;;
  test)
    exec /app/docker/run_all_modes_tests.sh "$@"
    ;;
  bash)
    exec /bin/bash "$@"
    ;;
  *)
    echo "Modo invalido: $mode"
    echo "Usa: console | gui | test | bash"
    exit 2
    ;;
esac

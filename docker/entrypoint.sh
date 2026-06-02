#!/usr/bin/env bash
set -euo pipefail

mode="${1:-console}"
shift || true

case "$mode" in
  console)
    # Si no hay TTY en stdin asumimos modo streaming (docker non-interactive)
    if [ ! -t 0 ]; then
      exec /app/progvoraz --docker "$@"
    else
      exec /app/progvoraz "$@"
    fi
    ;;
  gui)
    exec /app/progvoraz_gui "$@"
    ;;
  stream)
    exec /app/progvoraz --docker "$@"
    ;;
  server)
    exec /app/progvoraz --server "$@"
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

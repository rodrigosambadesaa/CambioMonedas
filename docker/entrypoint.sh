#!/usr/bin/env bash
set -euo pipefail

mode="${1:-console}"
shift || true

case "$mode" in
  console)
    exec /app/progvoraz "$@"
    ;;
  convert)
    # Ejecuta conversión no interactiva: args esperados: <from> <to> <amount_cents>
    exec /app/progvoraz --convert "$@"
    ;;
  convert-stock)
    # Igual que convert pero solicita uso de stock en destino
    exec /app/progvoraz --convert "$@" --convert-stock
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

#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="/app"
if [[ ! -d "$APP_DIR" ]]; then
  APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

# Runtime services mount the repository directory at /app/workspace so that
# stock.txt can be replaced atomically. Quality images keep their /app sandbox.
if [[ -f "$APP_DIR/workspace/monedas.txt" && -f "$APP_DIR/workspace/stock.txt" ]]; then
  cd "$APP_DIR/workspace"
fi

mode="${1:-console}"
shift || true

case "$mode" in
  console)
    exec "$APP_DIR/progvoraz" "$@"
    ;;
  convert)
    # Ejecuta conversión no interactiva: args esperados: <from> <to> <amount_cents>
    exec "$APP_DIR/progvoraz" --convert "$@"
    ;;
  convert-stock)
    # Igual que convert pero solicita uso de stock en destino
    exec "$APP_DIR/progvoraz" --convert "$@" --convert-stock
    ;;
  gui)
    exec "$APP_DIR/progvoraz_gui" "$@"
    ;;
  stream)
    exec "$APP_DIR/progvoraz" --docker "$@"
    ;;
  server)
    exec "$APP_DIR/progvoraz" --server "$@"
    ;;
  test)
    exec "$APP_DIR/scripts/quality_suite.sh" "$@"
    ;;
  smoke)
    exec "$APP_DIR/docker/run_all_modes_tests.sh" "$@"
    ;;
  memory)
    exec "$APP_DIR/scripts/memory_safety_suite.sh" "$@"
    ;;
  bash)
    exec /bin/bash "$@"
    ;;
  *)
    echo "Modo invalido: $mode"
    echo "Usa: console | convert | convert-stock | gui | stream | server | test | smoke | memory | bash"
    exit 2
    ;;
esac

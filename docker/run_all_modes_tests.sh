#!/usr/bin/env bash
set -euo pipefail

APP_DIR="/app"
cd "$APP_DIR"

currency="$(awk '/^[0-9-]+$/ { next } { print; exit }' monedas.txt)"
if [[ -z "$currency" ]]; then
  echo "No se encontro ninguna moneda valida en monedas.txt"
  exit 1
fi

stock_backup="$(mktemp)"
cp stock.txt "$stock_backup"

cleanup() {
  cp "$stock_backup" stock.txt || true
  rm -f "$stock_backup"
}
trap cleanup EXIT

run_case() {
  local name="$1"
  local exe="$2"
  local expected="$3"
  local input="$4"
  local output

  cp "$stock_backup" stock.txt
  output="$(printf "%b" "$input" | "$exe" 2>&1 || true)"

  if ! grep -Fq -- "$expected" <<<"$output"; then
    echo "[FALLO] $name"
    echo "Esperado: $expected"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  echo "[OK] $name"
}

run_case_updates_stock() {
  local name="$1"
  local exe="$2"
  local expected="$3"
  local input="$4"
  local output
  local before
  local after

  cp "$stock_backup" stock.txt
  before="$(cat stock.txt)"
  output="$(printf "%b" "$input" | "$exe" 2>&1 || true)"
  after="$(cat stock.txt)"

  if ! grep -Fq -- "$expected" <<<"$output"; then
    echo "[FALLO] $name"
    echo "Esperado: $expected"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  if [[ "$before" == "$after" ]]; then
    echo "[FALLO] $name"
    echo "Salida:"
    echo "$output"
    echo "stock.txt no cambio tras la operacion."
    exit 1
  fi

  echo "[OK] $name"
}

run_case "Consola modo a (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo o salir):" \
  "a\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola modo b (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo o salir):" \
  "b\n${currency}\ntradicional\n30\nsalir\n"

run_case_updates_stock "Consola modo b actualiza stock.txt" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo o salir):" \
  "b\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola modo b (restriccion por limite)" \
  ./progvoraz \
  "Restriccion de monedas (N, =N, N-M, volver, modo o salir):" \
  "b\n${currency}\n5\n30\n=2\nvolver\nmodo\nsalir\n"

run_case "Consola modo c (admin)" \
  ./progvoraz \
  "Accion admin (anadir/quitar/historial/resumen, volver, modo, salir):" \
  "c\n${currency}\nanadir\n1\n0\nsalir\n"

run_case "GUI portable modo limitado" \
  ./progvoraz_gui \
  "Accion (calcular/caja/limite/especifico/historial/resumen/anadir/quitar/modo/volver/salir):" \
  "limitado\n${currency}\ncalcular\n30\nsalir\n"

run_case "GUI portable modo ilimitado" \
  ./progvoraz_gui \
  "Accion (calcular/caja/limite/especifico/historial/resumen/modo/volver/salir):" \
  "ilimitado\n${currency}\ncalcular\n30\nsalir\n"

run_case "GUI portable historial desde modo" \
  ./progvoraz_gui \
  "--- HISTORIAL DE TRANSACCIONES ---" \
  "historial\n\nsalir\n"

echo "Todas las pruebas en contenedor han finalizado correctamente."

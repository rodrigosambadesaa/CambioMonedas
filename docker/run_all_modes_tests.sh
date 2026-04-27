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

  if ! grep -Fq "$expected" <<<"$output"; then
    echo "[FALLO] $name"
    echo "Esperado: $expected"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  echo "[OK] $name"
}

run_case "Consola modo a (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional/especifico, volver, modo o salir):" \
  "a\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola modo b (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional/especifico, volver, modo o salir):" \
  "b\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola modo c (admin)" \
  ./progvoraz \
  "Accion admin (anadir/quitar, volver, modo, salir):" \
  "c\n${currency}\nanadir\n1\n0\nsalir\n"

run_case "GUI portable modo limitado" \
  ./progvoraz_gui \
  "Accion (calcular/especifico/anadir/quitar/modo/volver/salir):" \
  "limitado\n${currency}\ncalcular\n30\nsalir\n"

run_case "GUI portable modo ilimitado" \
  ./progvoraz_gui \
  "Accion (calcular/especifico/modo/volver/salir):" \
  "ilimitado\n${currency}\ncalcular\n30\nsalir\n"

echo "Todas las pruebas en contenedor han finalizado correctamente."

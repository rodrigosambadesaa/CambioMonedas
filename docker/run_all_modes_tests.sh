#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="/app"
if [[ ! -d "$APP_DIR" ]]; then
  APP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
fi
cd "$APP_DIR"

if [[ ! -x ./progvoraz || ! -x ./progvoraz_gui ]]; then
  make clean
  make release
  make gui
fi

currency="$(awk '/^[0-9-]+$/ { next } { print; exit }' monedas.txt)"
if [[ -z "$currency" ]]; then
  echo "No se encontro ninguna moneda valida en monedas.txt"
  exit 1
fi

stock_backup="$(mktemp)"
cp stock.txt "$stock_backup"

report_backup="$(mktemp)"
if [[ -f reporte_global.txt ]]; then
  cp reporte_global.txt "$report_backup"
else
  : > "$report_backup"
fi

cleanup() {
  if [[ -n "${server_pid:-}" ]]; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
  cp "$stock_backup" stock.txt || true
  cp "$report_backup" reporte_global.txt || true
  rm -f "$stock_backup"
  rm -f "$report_backup" stock_snapshot.txt stock_out.json
  rm -f /tmp/progvoraz_batch_test.csv /tmp/progvoraz_batch_test.log /tmp/progvoraz_export_stock.json /tmp/progvoraz_export_report.txt
}
trap cleanup EXIT

run_case() {
  local name="$1"
  local exe="$2"
  local expected="$3"
  local input="$4"
  local output
  local status=0

  cp "$stock_backup" stock.txt
  output="$(printf "%b" "$input" | sh -lc "$exe" 2>&1)" || status=$?

  if ((status != 0)); then
    echo "[FALLO] $name (codigo de salida $status)"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

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
  local status=0

  cp "$stock_backup" stock.txt
  before="$(cat stock.txt)"
  output="$(printf "%b" "$input" | sh -lc "$exe" 2>&1)" || status=$?
  after="$(cat stock.txt)"

  if ((status != 0)); then
    echo "[FALLO] $name (codigo de salida $status)"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

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

run_case_contains_and_not_contains() {
  local name="$1"
  local exe="$2"
  local expected="$3"
  local forbidden="$4"
  local input="$5"
  local output
  local status=0

  cp "$stock_backup" stock.txt
  output="$(printf "%b" "$input" | sh -lc "$exe" 2>&1)" || status=$?

  if ((status != 0)); then
    echo "[FALLO] $name (codigo de salida $status)"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  if ! grep -Fq -- "$expected" <<<"$output"; then
    echo "[FALLO] $name"
    echo "Esperado: $expected"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  if grep -Fq -- "$forbidden" <<<"$output"; then
    echo "[FALLO] $name"
    echo "Se encontro texto no permitido: $forbidden"
    echo "Salida:"
    echo "$output"
    exit 1
  fi

  echo "[OK] $name"
}

run_file_case() {
  local name="$1"
  local exe="$2"
  local file_path="$3"
  local expected_file_text="$4"

  cp "$stock_backup" stock.txt
  if ! sh -lc "$exe" >/tmp/progvoraz_file_case.out 2>&1; then
    echo "[FALLO] $name"
    cat /tmp/progvoraz_file_case.out
    exit 1
  fi

  if [[ ! -f "$file_path" ]]; then
    echo "[FALLO] $name"
    echo "No se genero el archivo esperado: $file_path"
    cat /tmp/progvoraz_file_case.out
    exit 1
  fi

  if ! grep -Fq -- "$expected_file_text" "$file_path"; then
    echo "[FALLO] $name"
    echo "No se encontro el texto esperado en $file_path: $expected_file_text"
    echo "Contenido:"
    cat "$file_path"
    exit 1
  fi

  echo "[OK] $name"
}

run_server_case() {
  local port=18080
  local response=""

  ./docker/entrypoint.sh server --port "$port" >/tmp/progvoraz_server.out 2>&1 &
  server_pid=$!

  for _ in $(seq 1 30); do
    if response="$(curl --fail --silent --show-error "http://127.0.0.1:${port}/health" 2>/dev/null)"; then
      break
    fi
    sleep 0.1
  done

  if [[ "$response" != "OK" ]]; then
    echo "[FALLO] Entrypoint server responde en /health"
    cat /tmp/progvoraz_server.out
    exit 1
  fi

  kill "$server_pid"
  wait "$server_pid" 2>/dev/null || true
  server_pid=""
  echo "[OK] Entrypoint server responde en /health"
}

run_case "Consola modo a (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4, volver, modo, gui o salir):" \
  "a\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola lanza GUI desde comando global" \
  ./progvoraz \
  "Interfaz GUI lanzada en segundo plano." \
  "gui\nsalir\n"

run_case "Entrypoint console mantiene menu interactivo sin TTY" \
  "./docker/entrypoint.sh console" \
  "Conversion aplicada: 250 euro-centimos -> 273 dolar-centimos" \
  "x\neuro\n250\ndolar\nn\n\nsalir\n"

run_case "Consola modo b (tradicional)" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo, gui o salir):" \
  "b\n${currency}\ntradicional\n30\nsalir\n"

run_case_updates_stock "Consola modo b actualiza stock.txt" \
  ./progvoraz \
  "Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo, gui o salir):" \
  "b\n${currency}\ntradicional\n30\nsalir\n"

run_case "Consola modo b (restriccion por limite)" \
  ./progvoraz \
  "Restriccion de monedas (N, =N, N-M, volver, modo, gui o salir):" \
  "b\n${currency}\n5\n30\n=2\nvolver\nmodo\nsalir\n"

run_case "Consola operacion global snapshot" \
  ./progvoraz \
  "Snapshot de stock creado en stock_snapshot.txt" \
  "s\n\nsalir\n"

run_case "Consola operacion global reporte" \
  ./progvoraz \
  "Reporte global generado en reporte_global.txt" \
  "g\n\nsalir\n"

run_case "Consola modo c (admin)" \
  ./progvoraz \
  "Accion admin (anadir/quitar/historial/resumen, volver, modo, gui, salir):" \
  "c\n${currency}\nanadir\n1\n0\nsalir\n"

run_case "GUI portable modo limitado" \
  ./progvoraz_gui \
  "Accion (calcular/caja/limite/especifico/historial/resumen/snapshot/restaurar/reporte/json/anadir/quitar/modo/volver/gui/salir):" \
  "limitado\n${currency}\ncalcular\n30\nsalir\n"

run_case "GUI portable modo ilimitado" \
  ./progvoraz_gui \
  "Accion (calcular/caja/limite/especifico/historial/resumen/snapshot/restaurar/reporte/json/modo/volver/gui/salir):" \
  "ilimitado\n${currency}\ncalcular\n30\nsalir\n"

run_case "GUI portable historial desde modo" \
  ./progvoraz_gui \
  "--- HISTORIAL DE TRANSACCIONES ---" \
  "historial\n\nsalir\n"

run_case "GUI portable operacion global reporte" \
  ./progvoraz_gui \
  "Reporte global generado: reporte_global.txt" \
  "reporte\n\nsalir\n"

run_case "GUI portable operacion global json" \
  ./progvoraz_gui \
  "Stock exportado en JSON a stock_out.json" \
  "json\n\nsalir\n"

run_case "Consola conversion entre monedas" \
  "PROGVORAZ_EXCHANGE_SOURCE=stub ./progvoraz" \
  "Conversion aplicada: 250 euro-centimos -> 273 dolar-centimos" \
  "x\neuro\n250\ndolar\nn\n\nsalir\n"

run_case "GUI portable conversion entre monedas" \
  "PROGVORAZ_EXCHANGE_SOURCE=stub ./progvoraz_gui" \
  "Conversion aplicada: 250 euro-centimos -> 273 dolar-centimos" \
  "convertir\neuro\n250\ndolar\nn\n\nsalir\n"

run_case "Entrypoint stream sigue usando CSV Docker" \
  "./docker/entrypoint.sh stream" \
  "modo,moneda,monto,resultado,nota" \
  "mode,moneda,monto,range\na,euro,250,\n"

run_case "Entrypoint convert" \
  "PROGVORAZ_EXCHANGE_SOURCE=stub ./docker/entrypoint.sh convert euro dolar 250" \
  "Convert CLI euro->dolar | Origen=250 c | Destino=273 c | Tasa=1.09" \
  ""

run_case "Entrypoint convert-stock usa stock real" \
  "PROGVORAZ_EXCHANGE_SOURCE=stub ./docker/entrypoint.sh convert-stock euro dolar 250" \
  "cantidad 1 | stock" \
  ""

run_case "Entrypoint bash reenvia argumentos" \
  "./docker/entrypoint.sh bash -lc 'printf PROGVORAZ_BASH_OK'" \
  "PROGVORAZ_BASH_OK" \
  ""

run_case "Entrypoint console conserva metadatos de version" \
  "./docker/entrypoint.sh console --version" \
  "# Versiones y tags historicos" \
  ""

run_server_case

run_file_case "CLI batch genera CSV esperado" \
  "./progvoraz --input tests/sample_batch.csv --output /tmp/progvoraz_batch_test.csv --log /tmp/progvoraz_batch_test.log" \
  "/tmp/progvoraz_batch_test.csv" \
  "OK total=2"

run_file_case "CLI batch genera log esperado" \
  "./progvoraz --input tests/sample_batch.csv --output /tmp/progvoraz_batch_test.csv --log /tmp/progvoraz_batch_test.log" \
  "/tmp/progvoraz_batch_test.log" \
  "no solution"

run_file_case "CLI exporta stock JSON" \
  "./progvoraz --export-stock-json /tmp/progvoraz_export_stock.json" \
  "/tmp/progvoraz_export_stock.json" \
  "\"monedas\""

run_file_case "CLI exporta reporte global" \
  "./progvoraz --export-report /tmp/progvoraz_export_report.txt" \
  "/tmp/progvoraz_export_report.txt" \
  "Reporte global"

run_case "Consola modo ingles" \
  "PROGVORAZ_LANG=en ./progvoraz" \
  "Change option (traditional|1 / specific|2 / history|3 / summary|4, back, mode, gui or exit):" \
  "a\ndollar\ntraditional\n30\nexit\n"

big_digits="$(awk 'BEGIN { for (i = 0; i < 10000; ++i) printf "9" }')"

run_case_contains_and_not_contains "Consola admite bigint largo sin contaminar stdin" \
  ./progvoraz \
  "Gracias por utilizar este programa." \
  "Subopcion invalida." \
  "a\n${currency}\ntradicional\n${big_digits}\nsalir\n"

run_case "GUI portable modo ingles" \
  "PROGVORAZ_LANG=en ./progvoraz_gui" \
  "Action (calculate/register/limit/specific/history/summary/snapshot/restore/report/json/mode/back/gui/exit):" \
  "unlimited\ndollar\ncalculate\n30\nexit\n"

run_case "Entrypoint stream modo ingles" \
  "PROGVORAZ_LANG=en ./docker/entrypoint.sh stream" \
  "mode,currency,amount,result,note" \
  "mode,currency,amount,range\na,dollar,250,\n"

echo "Todas las pruebas en contenedor han finalizado correctamente."

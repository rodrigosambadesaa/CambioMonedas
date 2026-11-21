#!/usr/bin/env python3
"""Run the real CLI/portable GUI under ASan or Valgrind in isolated workspaces."""
import argparse
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("console")
parser.add_argument("gui")
parser.add_argument("--valgrind", action="store_true")
parser.add_argument("--asan", action="store_true")
args = parser.parse_args()
if args.valgrind == args.asan:
    parser.error("choose exactly one of --valgrind and --asan")

root = Path.cwd()
console = Path(args.console).resolve()
gui = Path(args.gui).resolve()
assert console.is_file() and gui.is_file()
prefix = (["valgrind", "--quiet", "--leak-check=full",
           "--show-leak-kinds=all", "--track-origins=yes",
           "--errors-for-leak-kinds=definite,possible",
           "--error-exitcode=99"] if args.valgrind else [])
env = os.environ.copy()
env["PROGVORAZ_EXCHANGE_SOURCE"] = "stub"
env["PROGVORAZ_LANG"] = "es"
if args.asan:
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
cases = [
    ("help", console, ["--help"], None, "Opciones", 0),
    ("version", console, ["--version"], None, "Versiones", 0),
    ("console-unlimited", console, [], "a\neuro\ntradicional\n30\nsalir\n", "Subopcion cambio", 0),
    ("console-stock", console, [], "b\neuro\ntradicional\n30\nsalir\n", "Subopcion cambio", 0),
    ("console-limit", console, [], "b\neuro\n5\n30\n=2\nvolver\nmodo\nsalir\n", "Restriccion de monedas", 0),
    ("console-admin", console, [], "c\neuro\nanadir\n1\n0\nsalir\n", "Accion admin", 0),
    ("console-report", console, [], "g\n\nsalir\n", "Reporte global generado", 0),
    ("console-snapshot", console, [], "s\n\nsalir\n", "Snapshot de stock", 0),
    ("console-very-large", console, [], "a\neuro\ntradicional\n" + "9"*1000 + "\nsalir\n", "Gracias por utilizar", 0),
    ("gui-unlimited", gui, [], "ilimitado\neuro\ncalcular\n30\nsalir\n", "Accion", 0),
    ("gui-limited", gui, [], "limitado\neuro\ncalcular\n30\nsalir\n", "Accion", 0),
    ("gui-history", gui, [], "historial\n\nsalir\n", "HISTORIAL", 0),
    ("gui-report", gui, [], "reporte\n\nsalir\n", "Reporte global", 0),
    ("gui-json", gui, [], "json\n\nsalir\n", "Stock exportado", 0),
    ("convert-forward", console, ["--convert", "EUR", "USD", "250"], None, "Destino=273", 0),
    ("convert-backward", console, ["--convert", "USD", "EUR", "109"], None, "Destino=100", 0),
    ("convert-big", console, ["--convert", "EUR", "USD", "1"+"0"*40], None, "Destino=", 0),
    ("convert-stock", console, ["--convert", "EUR", "USD", "250", "--convert-stock"], None, "Convert CLI", 0),
    ("convert-invalid", console, ["--convert", "EUR", "USD", "-12"], None, "no valido", 2),
    ("convert-rate-missing", console, ["--convert", "EUR", "JPY", "100"], None, "No se pudo obtener", 2),
    ("convert-args-missing", console, ["--convert", "EUR", "USD"], None, "--convert requiere", 2),
    ("stream", console, ["--stream"], "mode,currency,amount,range\na,euro,250,\nb,euro,127,\na,NOPE,10,\na,euro,,\n", "modo,moneda,monto,resultado,nota", 0),
    ("legacy-docker", console, ["--docker"], "mode,currency,amount,range\na,euro,100,\n", "modo,moneda,monto,resultado,nota", 0),
    ("batch", console, ["--input", str(root/"tests/sample_batch.csv"), "--output", "output.csv", "--log", "run.log"], None, "", 0),
    ("json-export", console, ["--export-stock-json", "stock.json"], None, "Stock exportado", 0),
    ("report-export", console, ["--export-report", "report.txt"], None, "Reporte global", 0),
    ("batch-missing-file", console, ["--input", "missing.csv"], None, "", 2),
]
if args.asan:
    # ASan/LSan is exercised on every path, while Valgrind is the independent leak oracle.
    pass

with tempfile.TemporaryDirectory(prefix="progvoraz-memory-audit-") as tmp:
    for name, exe, options, stdin, expected, status in cases:
        work = Path(tmp) / name
        work.mkdir()
        for source in ("monedas.txt", "stock.txt", "rates_stub.json", "VERSIONES.md"):
            shutil.copy2(root/source, work/source)
        command = [*prefix, str(exe), *options]
        result = subprocess.run(command, cwd=work, input=stdin, capture_output=True,
                                text=True, env=env, timeout=150)
        combined = result.stdout + result.stderr
        if result.returncode != status or (expected and expected.lower() not in combined.lower()):
            raise AssertionError(f"{name}: expected exit={status}, got={result.returncode}; "
                                 f"stdout={result.stdout[-1500:]!r}; stderr={result.stderr[-2500:]!r}")
        if name == "batch" and not (work/"output.csv").is_file():
            raise AssertionError("Batch failed to produce output.csv")
        if name == "json-export" and not (work/"stock.json").is_file():
            raise AssertionError("JSON export missing")
        if name == "report-export" and not (work/"report.txt").is_file():
            raise AssertionError("Report export missing")
        print(f"PASS {name}", flush=True)

print(f"PASS: {len(cases)} independent runtime paths ({'Valgrind' if args.valgrind else 'ASan/LSan/UBSan'})")

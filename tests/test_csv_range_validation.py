#!/usr/bin/env python3
"""Reject corrupted range strings consistently in batch and streaming CSV."""
import csv
import io
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = (ROOT / "progvoraz").resolve()
invalid = ["abc", "1junk", "+1", "-1", "1-", "-2", "1-2-3",
           "3-1", "1--2", "18446744073709551615",
           "18446744073709551616", "9" * 60]
valid = ["", "1", "0-3", "1-3", "2-2"]
ranges = invalid + valid
payload = ("mode,currency,amount,range\n" +
           "".join(f"a,euro,3,{v}\n" for v in ranges))

with tempfile.TemporaryDirectory(prefix="c-csv-range-") as temp:
    work = pathlib.Path(temp)
    for name in ("monedas.txt", "stock.txt", "VERSIONES.md", "rates_stub.json"):
        shutil.copy2(ROOT/name, work/name)

    for kind in ("stream", "batch"):
        if kind == "stream":
            cmd = [str(EXE), "--stream"]
            stdin = payload
        else:
            (work / "cases.csv").write_text(payload)
            cmd = [str(EXE), "--input", "cases.csv", "--output", "results.csv"]
            stdin = None
        proc = subprocess.run(cmd, input=stdin, capture_output=True, text=True,
                              cwd=work, timeout=40)
        assert proc.returncode == 0, (kind, proc.stderr[-1200:])
        content = proc.stdout if kind == "stream" else (work / "results.csv").read_text()
        rows = list(csv.reader(io.StringIO(content)))
        assert len(rows) == len(ranges) + 1, (kind, rows)
        for value, row in zip(ranges, rows[1:]):
            assert len(row) == 5, (kind, value, row)
            if value in invalid:
                assert row[3] == "ERROR" and "rango" in row[4].lower(), \
                    (kind, value, row)
            else:
                assert row[3].startswith("OK total=") or row[3] == "NO", \
                    (kind, value, row)
        print(f"PASS {kind}: {len(invalid)} invalid ranges rejected, "
              f"{len(valid)} valid ranges kept")

print("PASS C CSV range parser rejects overflow and bypasses")

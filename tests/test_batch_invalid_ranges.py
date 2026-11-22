#!/usr/bin/env python3
"""Invalid CSV range columns must be rejected in stream and batch modes."""
import csv
import io
import pathlib
import subprocess
import sys
import tempfile

binary=pathlib.Path(sys.argv[1]).resolve()
bad=["garbage","2extra","-1","2--3","9-3","0-1more","1-",
     "9999999999999999999999999999","1-9999999999999999999999999",
     "+2","2-3-4","1.5","1--1","1-0"]
good=["0-3","2"," 0 - 3 ",""]
source="mode,currency,amount,range\n"+"".join(
    f"a,EUR,6,{value}\n" for value in bad+good)

def verify(text):
    rows=list(csv.reader(io.StringIO(text)))
    assert len(rows)==len(bad)+len(good)+1, rows
    for raw,row in zip(bad,rows[1:1+len(bad)]):
        assert len(row)==5 and row[3]=="ERROR" and "rango" in row[4].lower(),(raw,row)
    for raw,row in zip(good,rows[1+len(bad):]):
        assert len(row)==5 and row[3]!="ERROR",(raw,row)

stream=subprocess.run([str(binary),"--stream"],input=source,
    capture_output=True,text=True,timeout=40)
assert stream.returncode==0,stream.stderr
verify(stream.stdout)
with tempfile.TemporaryDirectory() as temp:
    root=pathlib.Path(temp)
    (root/"input.csv").write_text(source,encoding="utf-8")
    batch=subprocess.run([str(binary),"--input",str(root/"input.csv"),
        "--output",str(root/"output.csv")],capture_output=True,text=True,timeout=40)
    assert batch.returncode==0,batch.stderr
    verify((root/"output.csv").read_text(encoding="utf-8"))
print(f"PASS {len(bad)} invalid + {len(good)} valid ranges in both CSV modes")

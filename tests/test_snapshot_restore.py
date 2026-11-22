#!/usr/bin/env python3
"""Atomic snapshot rollback, validation and cross-process stock lock."""
import fcntl
from pathlib import Path
import subprocess
import tempfile
import time

BIN = (Path.cwd() / "snapshot_cli").resolve()
DENOMS = "euro\n5\n1\ndolar\n10\n1\n"
ORIGINAL = "euro\n4\n5\ndolar\n8\n9\n"
VALID = "euro\n2\n3\ndolar\n5\n6\n"
INVALID = [
    "euro\n2\n3\ndolar\n5\n",
    "euro\n2\n3\n",
    "euro\n2\n3\ndolar\n5\n6\nunknown\n1\n",
    "euro\n2\n3\ndolar\ninvalid\n6\n",
]
with tempfile.TemporaryDirectory(prefix="snapshot-atomic-") as temp:
    root = Path(temp)
    (root / "monedas.txt").write_text(DENOMS)
    live = root / "stock.txt"
    live.write_text(ORIGINAL)
    snapshot = root / "snapshot.txt"
    snapshot.write_text(VALID)
    for bad in INVALID:
        candidate = root / "invalid.txt"
        candidate.write_text(bad)
        result = subprocess.run([str(BIN), "restore", str(candidate)], cwd=root,
                                capture_output=True, timeout=10)
        assert result.returncode != 0, (bad, result.stderr)
        assert live.read_text() == ORIGINAL, "corrupt snapshot changed inventory"
    assert not list(root.glob("stock.txt.tmp*")), "orphan temporary after failure"

    # Lock out normal C/C++/C# stock writers with the standard sidecar lock.
    with (root / "stock.txt.lock").open("a+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        proc = subprocess.Popen([str(BIN), "restore", str(snapshot)], cwd=root)
        time.sleep(.3)
        assert proc.poll() is None, "snapshot restore bypassed interprocess lock"
        assert live.read_text() == ORIGINAL
        fcntl.flock(lock, fcntl.LOCK_UN)
    assert proc.wait(timeout=10) == 0
    assert live.read_text() == VALID, "snapshot not restored exactly"
    assert not list(root.glob("stock.txt.tmp*")), "orphan stage after success"

    copied = root / "copy.txt"
    assert subprocess.run([str(BIN), "snapshot", str(copied)], cwd=root).returncode == 0
    assert copied.read_text() == VALID
    print("PASS: C snapshot rollback, validation, atomic swap, interprocess lock")

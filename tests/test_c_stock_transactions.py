#!/usr/bin/env python3
"""Concurrent inventory-writing and external-lock regression for C."""
import concurrent.futures
import contextlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

if os.name != "nt":
    import fcntl


@contextlib.contextmanager
def exclusive_lock(path):
    if os.name != "nt":
        with path.open("a+b") as f:
            fcntl.flock(f, fcntl.LOCK_EX)
            try:
                yield
            finally:
                fcntl.flock(f, fcntl.LOCK_UN)
    else:
        import ctypes
        from ctypes import wintypes
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        create = kernel.CreateFileW
        create.argtypes = [
            wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
            ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE
        ]
        create.restype = wintypes.HANDLE
        close = kernel.CloseHandle
        close.argtypes = [wintypes.HANDLE]
        close.restype = wintypes.BOOL
        handle = create(str(path), 0xC0000000, 0, None, 4, 0x80, None)
        assert handle != ctypes.c_void_p(-1).value, ctypes.get_last_error()
        try:
            yield
        finally:
            assert close(handle)


def run(executable, cwd, currency, amount, *extras):
    proc = subprocess.run(
        [str(executable), currency, str(amount), *extras],
        cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, timeout=30,
    )
    assert proc.returncode == 0, (
        currency, amount, extras, proc.returncode, proc.stdout, proc.stderr
    )


def parse(path):
    blocks = {}
    key = None
    for line in path.read_text(encoding="utf-8").splitlines():
        if line in ("euro", "dolar"):
            assert line not in blocks, line
            blocks[line] = []
            key = line
        else:
            assert key is not None and line.lstrip("-").isdigit(), line
            blocks[key].append(0 if line == "-1" else int(line))
    return blocks


def main():
    assert len(sys.argv) == 2
    executable = Path(sys.argv[1]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="progvoraz-atomic-stock-") as directory:
        root = Path(directory)
        (root / "monedas.txt").write_text(
            "euro\n5\n3\n1\ndolar\n10\n5\n1\n", encoding="utf-8"
        )
        stock = root / "stock.txt"
        stock.write_text(
            "euro\n10\n10\n10\ndolar\n10\n10\n10\n", encoding="utf-8"
        )
        lockfile = root / "stock.txt.lock"

        # The C writer must respect a lock held by an unrelated process,
        # using the same protocol as the existing C++ and C# implementations.
        with exclusive_lock(lockfile):
            process = subprocess.Popen(
                [str(executable), "euro", "111"], cwd=root,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            )
            time.sleep(0.3)
            assert process.poll() is None, "C ignored the cross-language stock lock"
            assert parse(stock)["euro"] == [10, 10, 10], "Wrote while locked"
        stdout, stderr = process.communicate(timeout=30)
        assert process.returncode == 0, (stdout, stderr)
        assert parse(stock)["euro"] == [111] * 3

        # Different-currency transactions must preserve ALL currency blocks
        # despite concurrent read-modify-write and unique staging files.
        huge = 10**42 + 321
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            futures = [
                pool.submit(run, executable, root,
                            "euro" if i % 2 == 0 else "dolar",
                            huge if i % 2 == 0 else 77)
                for i in range(24)
            ]
            for task in concurrent.futures.as_completed(futures):
                task.result()
        assert parse(stock) == {"euro": [huge] * 3, "dolar": [77] * 3}

        before = stock.read_bytes()
        run(executable, root, "euro", 123, "--short")
        assert stock.read_bytes() == before, "Invalid denomination count changed stock"
        failed = subprocess.run(
            [str(executable), "euro", "7" * 260], cwd=root, timeout=30,
            capture_output=True,
        )
        assert failed.returncode != 0, "Overlong stock entry should be rejected"
        assert stock.read_bytes() == before, "Rejected update modified inventory"
        assert not list(root.glob("stock.txt.tmp*")), "Orphan staging files"
        print("PASS: C atomic inventory persistence, interprocess locks, 24 writers and rollback")


if __name__ == "__main__":
    main()

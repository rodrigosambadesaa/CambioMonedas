#!/usr/bin/env python3
"""HTTP memory/FD/tempfile regression suite (real network requests, finite server)."""
import argparse
import http.client
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser()
parser.add_argument("binary")
parser.add_argument("--valgrind", action="store_true")
args = parser.parse_args()
root = Path.cwd()
binary = Path(args.binary).resolve()
if not binary.is_file():
    raise SystemExit("server binary does not exist")

def request(port, method, path, body=None, headers=None):
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=12)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        result = conn.getresponse()
        return result.status, result.read()
    finally:
        conn.close()

def raw_request(port, payload):
    conn = socket.create_connection(("127.0.0.1", port), timeout=12)
    conn.settimeout(12)
    try:
        conn.sendall(payload)
        conn.shutdown(socket.SHUT_WR)
        chunks = []
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break
            chunks.append(chunk)
        return b"".join(chunks)
    finally:
        conn.close()

with tempfile.TemporaryDirectory(prefix="progvoraz-http-audit-") as t:
    work = Path(t)
    for filename in ("monedas.txt", "stock.txt", "rates_stub.json", "VERSIONES.md"):
        shutil.copy2(root / filename, work / filename)
    holder = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    holder.bind(("127.0.0.1", 0))
    port = holder.getsockname()[1]
    holder.close()

    before = set(Path("/tmp").glob("progvoraz_in_*")) | set(Path("/tmp").glob("progvoraz_out_*"))
    before |= set(Path("/tmp").glob("progvoraz_export_stock_*"))
    env = os.environ.copy()
    env["PROGVORAZ_HTTP_MAX_REQUESTS"] = "40"
    env["PROGVORAZ_EXCHANGE_SOURCE"] = "stub"
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    command = [str(binary), "--server", "--port", str(port)]
    if args.valgrind:
        command = ["valgrind", "--quiet", "--leak-check=full",
                   "--show-leak-kinds=all", "--track-origins=yes",
                   "--errors-for-leak-kinds=definite,possible",
                   "--error-exitcode=99", *command]
    logpath = work / "server.log"
    with logpath.open("wb") as log:
        proc = subprocess.Popen(command, cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            for i in range(240):
                try:
                    status, body = request(port, "GET", "/health")
                    break
                except (ConnectionError, TimeoutError, OSError):
                    if proc.poll() is not None:
                        raise AssertionError(f"Server exited early ({proc.returncode}): {logpath.read_text()}")
                    time.sleep(.05)
            else:
                raise AssertionError("Server did not accept connections")
            assert status == 200 and body == b"OK", (status, body)
            print("PASS health/readiness", flush=True)
            fd_path = Path(f"/proc/{proc.pid}/fd")
            baseline_fds = len(list(fd_path.iterdir())) if fd_path.is_dir() else None

            status, body = request(port, "GET", "/export_stock_json")
            assert status == 200 and "monedas" in json.loads(body), (status, body[:100])
            print("PASS JSON export", flush=True)

            status, body = request(port, "GET", "/not-a-real-path")
            assert status == 404, (status, body)
            print("PASS 404", flush=True)

            csv = b"mode,currency,amount,range\na,euro,250,\n"
            status, body = request(port, "POST", "/batch", csv, {"Content-Type": "text/csv"})
            assert status == 200 and b"OK total=" in body, (status, body)
            print("PASS batch", flush=True)

            invalid = b"mode,currency,amount,range\na,INVALID,123,\na,euro,INVALID,\n"
            status, body = request(port, "POST", "/batch", invalid, {"Content-Type": "text/csv"})
            assert status == 200 and b"ERROR" in body, (status, body)
            print("PASS invalid batch", flush=True)

            result = raw_request(port,
                b"POST /batch HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\nbroken")
            # Closing a truncated body must free the partially allocated heap buffer.
            assert not result or result.startswith(b"HTTP/"), result[:100]
            print("PASS truncated POST", flush=True)

            result = raw_request(port,
                b"POST /batch HTTP/1.1\r\nHost: localhost\r\n"
                b"Content-Length: 18446744073709551615\r\n\r\n")
            assert result.startswith(b"HTTP/1.1 413"), result[:150]
            print("PASS enormous Content-Length rejected", flush=True)

            result = raw_request(port,
                b"POST /batch HTTP/1.1\r\nHost: localhost\r\n"
                b"Content-Length: nonsense\r\n\r\n")
            assert result.startswith(b"HTTP/1.1 400"), result[:150]
            print("PASS malformed Content-Length rejected", flush=True)

            # Thirty-two more requests, total exactly forty; the server must
            # exit normally so LSan/Valgrind can inspect surviving allocations.
            for i in range(32):
                status, body = request(port, "GET", "/health")
                assert (status, body) == (200, b"OK"), (i, status, body)
            print("PASS 32 repeated requests", flush=True)
            if baseline_fds is not None:
                current_fds = len(list(fd_path.iterdir()))
                assert current_fds <= baseline_fds + 2, (
                    f"Open file descriptors grew from {baseline_fds} to {current_fds}")
                print(f"PASS fd stability ({baseline_fds} -> {current_fds})", flush=True)

            rc = proc.wait(timeout=120)
            logtext = logpath.read_text(errors="replace")
            assert rc == 0, f"Server exit={rc}, tool log:\n{logtext[-8000:]}"
            print("PASS normal process exit and memory checker", flush=True)

            after = set(Path("/tmp").glob("progvoraz_in_*")) | set(Path("/tmp").glob("progvoraz_out_*"))
            after |= set(Path("/tmp").glob("progvoraz_export_stock_*"))
            leaked = sorted(map(str, after - before))
            assert not leaked, f"Temporary-file leak after HTTP requests: {leaked}"
            print("PASS temporary files removed", flush=True)
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=10)

print(f"PASS HTTP resource audit ({'Valgrind' if args.valgrind else 'ASan/LSan/UBSan'})")

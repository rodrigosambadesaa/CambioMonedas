#!/usr/bin/env python3
"""HTTP field names and Content-Length must never be parsed as substrings."""
from pathlib import Path
import http.client
import os
import socket
import subprocess
import sys
import time

exe=Path(sys.argv[1]).resolve(strict=True)
with socket.socket() as s:
    s.bind(("127.0.0.1",0))
    port=s.getsockname()[1]
env=os.environ.copy()
env["PROGVORAZ_HTTP_MAX_REQUESTS"]="7"
env.pop("PROGVORAZ_HTTP_BIND_ALL",None)
env["ASAN_OPTIONS"]="detect_leaks=1:halt_on_error=1:abort_on_error=1"
env["UBSAN_OPTIONS"]="halt_on_error=1:print_stacktrace=1"
p=subprocess.Popen([str(exe),"--server","--port",str(port)],
                   stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env)
def get_health():
    connection=http.client.HTTPConnection("127.0.0.1",port,timeout=5)
    try:
        connection.request("GET","/health")
        resp=connection.getresponse()
        return resp.status,resp.read()
    finally:
        connection.close()
def raw(payload):
    with socket.create_connection(("127.0.0.1",port),timeout=5) as sock:
        sock.settimeout(5)
        sock.sendall(payload)
        sock.shutdown(socket.SHUT_WR)
        data=bytearray()
        while True:
            chunk=sock.recv(8192)
            if not chunk: break
            data.extend(chunk)
        return bytes(data)
try:
    for _ in range(100):
        try:
            if get_health()==(200,b"OK"): break
        except OSError:
            if p.poll() is not None: raise AssertionError("server exited early")
            time.sleep(.05)
    else: raise AssertionError("server never started")
    prefix=b"POST /batch HTTP/1.1\r\nHost: localhost\r\n"
    # X-Content-Length must not be mistaken for a proper field.
    assert raw(prefix+b"X-Content-Length: 5\r\n\r\nhello").startswith(b"HTTP/1.1 411")
    # Even identical duplicated lengths are rejected to avoid ambiguity.
    assert raw(prefix+b"Content-Length: 2\r\ncontent-length: 2\r\n\r\n").startswith(b"HTTP/1.1 400")
    # The listener has no chunked decoder.
    assert raw(prefix+b"Transfer-Encoding: chunked\r\n\r\n").startswith(b"HTTP/1.1 400")
    assert raw(prefix+b"Content-Length: 2junk\r\n\r\n").startswith(b"HTTP/1.1 400")
    valid=b"mode,currency,amount,range\na,euro,6,\n"
    answer=raw(prefix+b"Content-Length: "+str(len(valid)).encode()+b"\r\n\r\n"+valid)
    assert answer.startswith(b"HTTP/1.1 200") and b"OK total=" in answer,answer[:500]
    assert get_health()==(200,b"OK")
    stdout,_=p.communicate(timeout=20)
    assert p.returncode==0,(p.returncode,stdout[-4000:])
finally:
    if p.poll() is None:
        p.kill()
        p.communicate(timeout=5)
print("PASS: exact Content-Length field, duplicate/chunked rejection and HTTP recovery")

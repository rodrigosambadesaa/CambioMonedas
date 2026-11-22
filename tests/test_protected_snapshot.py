#!/usr/bin/env python3
"""C backup API must reject stock.txt and aliases before truncating anything."""
import pathlib
import subprocess
import sys
import tempfile

def main():
    executable = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as temp:
        d = pathlib.Path(temp)
        (d / "monedas.txt").write_text("euro\n5\n1\n")
        data = b"euro\n7\n13\n"
        (d / "stock.txt").write_bytes(data)
        for item in ("stock.txt", "./stock.txt", "monedas.txt"):
            before = (d / "monedas.txt").read_bytes()
            p = subprocess.run([str(executable), "snapshot", item],
                               cwd=d, capture_output=True, timeout=20)
            assert p.returncode != 0, (item, p.stderr)
            assert (d / "stock.txt").read_bytes() == data
            assert (d / "monedas.txt").read_bytes() == before
        for kind in ("symlink", "hardlink"):
            target = d / (kind + ".txt")
            if kind == "symlink":
                target.symlink_to(d / "stock.txt")
            else:
                target.hardlink_to(d / "stock.txt")
            p = subprocess.run([str(executable), "snapshot", str(target)],
                               cwd=d, capture_output=True, timeout=20)
            assert p.returncode != 0, (kind, p.stderr)
            assert (d / "stock.txt").read_bytes() == data
            target.unlink()
        backup=d/"backup.txt"
        p=subprocess.run([str(executable), "snapshot", str(backup)],
                         cwd=d, capture_output=True, timeout=20)
        assert p.returncode==0 and backup.read_bytes()==data
        # Atomic replacement: an existing valid backup must remain intact if
        # the source becomes unreadable/missing midway through a new backup.
        live = d / "stock.txt"
        live.rename(d / "stock-disabled.txt")
        p = subprocess.run([str(executable), "snapshot", str(backup)],
                           cwd=d, capture_output=True, timeout=20)
        assert p.returncode != 0, (p.returncode, p.stderr)
        assert backup.read_bytes() == data, "existing backup destroyed by failed copy"
        (d / "stock-disabled.txt").rename(live)
        assert not list(d.glob("backup.txt.tmp.*")), "orphan staging files"
        newer = b"euro" + bytes([10]) + b"9" + bytes([10]) + b"11" + bytes([10])
        live.write_bytes(newer)
        p=subprocess.run([str(executable), "snapshot", str(backup)],
                         cwd=d, capture_output=True, timeout=20)
        assert p.returncode==0 and backup.read_bytes()==newer
        assert not list(d.glob("backup.txt.tmp.*")), "orphan staging after atomic update"
        print("PASS: snapshot source protected against same path, relative aliases and hardlinks")

if __name__ == "__main__":
    main()

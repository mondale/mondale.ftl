#!/usr/bin/env python3

import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    ignored_dirs = {"plugins", "plz-out"}
    build_files = [
        p
        for p in Path(".").rglob("BUILD")
        if not any(part in ignored_dirs for part in p.parts)
    ]

    if not build_files:
        return

    for build_file in build_files:
        tmp_file = tempfile.NamedTemporaryFile(
            mode="w", delete=False, encoding="utf-8", dir=build_file.parent
        )
        tmp_path = Path(tmp_file.name)
        tmp_file.close()

        try:
            with open(tmp_path, "w", encoding="utf-8") as f:
                subprocess.run(
                    ["plz", "fmt", str(build_file)],
                    stdout=f,
                    check=True,
                    text=True,
                )
            
            if tmp_path.stat().st_size > 0:
                tmp_path.replace(build_file)
            else:
                print(f"Formatter output for {build_file} was empty; skipping replacement.", file=sys.stdout)
        except subprocess.CalledProcessError as e:
            print(f"Error formatting {build_file}: {e}", file=sys.stderr)
            sys.exit(1)
        finally:
            if tmp_path.exists():
                tmp_path.unlink()


if __name__ == "__main__":
    main()

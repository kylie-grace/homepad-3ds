#!/bin/sh
set -eu

echo "== HomePad 3DS Portainer test =="
echo "workspace: $(pwd)"

if command -v python3 >/dev/null 2>&1; then
  python3 tools/verify_config.py
else
  echo "python3 not found; skipping config validation in this container"
fi

make clean
make

test -s homepad.3dsx
test -s homepad.elf
test -s homepad.smdh

mkdir -p /artifacts
cp -f homepad.3dsx homepad.elf homepad.smdh /artifacts/

echo "== Artifacts =="
ls -lh /artifacts
echo "PASS HomePad build artifacts created"

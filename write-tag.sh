#!/usr/bin/env bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo 'ERROR: Expected 1 argument (path)' > /dev/stderr
    exit 1
fi

tmp=$(mktemp)
nfc-mfultralight r "$tmp"
printf '%s\x00' "$1" | dd bs=1 of="$tmp" seek=16 conv=notrunc
yes N | nfc-mfultralight w "$tmp"

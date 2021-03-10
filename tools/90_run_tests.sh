#!/bin/sh

set -e
set -u

cleanup() {
	${ncat_pid+kill ${ncat_pid}}
}
trap cleanup EXIT INT TERM

# setup fake ads server and install cleanup trap
ncat -l 48898 --keep-open &
ncat_pid=$!

# wait for fake ads server to accept connections
while ! ncat --send-only localhost 48898; do sleep 1; done

# transitional hack to test meson or legacy build binaries
ln -s build/AdsLibTest AdsLibTest.bin || true
ln -s build/AdsLibOOITest AdsLibOOITest.bin || true
ln -s build/example example/example.bin || true

./AdsLibTest.bin
./AdsLibOOITest.bin
./example/example.bin

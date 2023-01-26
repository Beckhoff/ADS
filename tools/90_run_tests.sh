#!/bin/sh

set -e
set -u

cleanup() {
	set +e
	${ncat_pid+kill ${ncat_pid}}
	${socat_pid+kill ${socat_pid}}
}
trap cleanup EXIT INT TERM

readonly script_path="$(cd "$(dirname "$0")" && pwd)"

"${script_path}/80_ads_route.sh"
"${script_path}/91_test_AdsTool.sh"

# Setup socat port forwarding to test variable ADS ports. We have no socat on
# Windows and not yet on TC/BSD, but we want to make sure we test on Linux,
# where socat should be available!
if test "Linux" = "$(uname)"; then
	socat TCP-LISTEN:12345,fork TCP:ads-server:48898 &
	socat_pid=$!
	while ! nc -z localhost 12345; do
		sleep 1
		echo waiting for socat
	done
	${QEMU_USER_EMULATION-} ./build/adstool "$(${QEMU_USER_EMULATION-} ./build/adstool ads-server netid)" --gw=127.0.0.1:12345 --localams="$(ip_route_src ads-server).1.1" license systemid
fi

# setup fake ads server and install cleanup trap
nc -l 48898 -k &
ncat_pid=$!

# wait for fake ads server to accept connections
while ! nc -z localhost 48898; do sleep 1; done

${QEMU_USER_EMULATION-} ./build/AdsLibTest
${QEMU_USER_EMULATION-} ./build/AdsLibOOITest
${QEMU_USER_EMULATION-} ./example/build/example

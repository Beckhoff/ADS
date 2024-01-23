#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2022 Beckhoff Automation GmbH & Co. KG

set -e
set -u

script_path="$(cd "$(dirname "$0")" && pwd)"
. "${script_path}/../lib_ssh_test.sh"

# Copy all necessary files from test-runner container into the test-device.
scp -r \
	"${script_path}/../../build" \
	"${script_path}/../../debian" \
	"${script_path}/../../example" \
	"${script_path}/../../tools" \
	"Administrator@${test_device}:"

# Run our generic tests within the QEMU test-device. For this to work we have
# to use the IP of the VM host, because the ADS ports are forwarded directly
# into the VM.
if ! container_ip="$(ip_route_src ads-server)"; then
	printf 'Failed to detect container IP.\n' >&2
	exit 1
fi
${ssh_cmd} ./tools/90_run_tests.sh "${container_ip}"

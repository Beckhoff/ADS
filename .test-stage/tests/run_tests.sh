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

# Run our generic tests within the QEMU test-device.
${ssh_cmd} /bin/sh -$- <<- EOF
	export BHF_CI_NAT_IP="$(ip_route_src ads-server)"
	./tools/90_run_tests.sh
EOF

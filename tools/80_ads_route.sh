#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 - 2022 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

add_route_ads() {
	local _container_ip
	_container_ip="$(ip_route_src "${1}")"
	echo "CONTAINER_IP: ${_container_ip}"

	${QEMU_USER_EMULATION-} ./build/adstool "${test_device}" --retry=9 addroute "--netid=${_container_ip}.1.1" "--addr=${BHF_CI_NAT_IP-${_container_ip}}" --password=1
}

set -e
set -u

readonly script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly test_device=ads-server

add_route_ads "${test_device}"

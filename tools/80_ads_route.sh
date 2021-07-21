#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

add_route_ads() {
	local _container_ip
	_container_ip="$(ip_route_src "${1}")"
	echo "CONTAINER_IP: ${_container_ip}"
	while ! ${script_path}/add-route.py --route_name "Testroute" --sender_ams "${_container_ip}.1.1" --route_dest "${_container_ip}" --plc_username Administrator --plc_password 1 --plc_ip "${test_device}"; do
		echo "Adding ads route ..."
		sleep 1
	done
}

host_as_ip() {
	local _host="${1}"
	{
		# getent hosts will fail for ip or bad hostnames. In case _host was already
		# an ip address we simply print it again. This would pass back a bad
		# hostname, too, but we can't easily distingish here so let us fail later.
		getent hosts "${_host}" || printf '%s' "${_host}"
	} | cut -f1 -d" "
}

install_pyads() {
	if ! python3 -c "import pyads"; then
		if ! command -v pip3; then
			apt update -y && apt install -y python3-pip
		fi
		pip3 install pyads
	fi
}

set -e
set -u

readonly script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly test_device=ads-server

install_pyads
add_route_ads "$(host_as_ip "${test_device}")"

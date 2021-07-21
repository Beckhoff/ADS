#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

my_ip() {
	local _dest_ip="${1}"
	if ! command -v ip_route_src > /dev/null; then
		ifconfig "$(route get "${_dest_ip}" | grep "interface:" | cut -d' ' -f4)" inet | tr "[:blank:]" "\n" | awk '/inet|src/{getline; print}'
	else
		ip_route_src "${_dest_ip}"
	fi
}

add_route_ads() {
	local _container_ip
	_container_ip="$(my_ip "${1}")"
	echo "CONTAINER_IP: ${_container_ip}"

	while ! ./build/adstool "${test_device}" addroute "--netid=${_container_ip}.1.1" "--addr=${BHF_CI_NAT_IP-${_container_ip}}" --password=1; do
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

set -e
set -u

readonly script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly test_device=ads-server

add_route_ads "$(host_as_ip "${test_device}")"

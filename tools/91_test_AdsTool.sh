#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

cleanup() {
	rm -f "${tmpfile-}"
}

as_hex() {
	if command -v tac > /dev/null; then
		local _tac_cmd='tac'
	else
		local _tac_cmd='tail -r'
	fi
	printf '0x'
	xxd -p -c1 "${1}" | ${_tac_cmd} | xxd -p -r | hexdump -v -e '/1 "%02X"'
}

check() {
	"${ads_tool}" "${netid}" "--gw=${remote}" "$@"
}

check_raw() {
	local _test_string
	_test_string="$(date +%F%T)"
	readonly _test_string

	# get symbol handle from PLC, written as binary to file
	printf 'MAIN.moreBytes' | check raw --read=4 0xF003 0 > "${tmpfile}"

	# use symbol handle from binary as index offset to write to variable by handle
	printf '%s' "${_test_string}" | check raw 0xF005 "$(as_hex "${tmpfile}")"

	# read back the written timestamp 0xF005 == 61445
	local _response
	_response="$(check raw --read=255 61445 "$(as_hex "${tmpfile}")")"
	if ! test "${_test_string}" = "${_response}"; then
		printf 'check_raw() mismatch:\n>%s<\n>%s<\n' "${_test_string}" "${_response}" >&2
		return 1
	fi
}

check_state() {
	if ! check state; then
		printf 'check_state() could not read anything\n' >&2
		return 1
	fi

	# This might disconnect our ADS connection, so we ignore that expected error and wait a short moment
	check state "${ADSSTATE_RESET}" || true
	sleep 2

	# Now, wait until TwinCAT reaches RUN
	local _state
	_state="$(check state)"
	local _retries=0
	while ! check state | grep -E "^(${ADSSTATE_RUN})\$"; do
		retries=$((retries + 1))
		if ! test "${_retries}" -lt "${BHF_CI_ADS_RETRY_LIMIT}"; then
			return 1
		fi
		printf 'state is %s waiting for %s\n' "${_state}" "${ADSSTATE_RUN}" >&2
		sleep 1
		_state="$(check state)"
	done
}

set -e
set -u

readonly BHF_CI_ADS_RETRY_LIMIT="${BHF_CI_ADS_RETRY_LIMIT-10}"
readonly ADSSTATE_RESET=2
readonly ADSSTATE_RUN=5

readonly script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly remote="${1-ads-server}"
readonly ads_tool="${2-${script_path}/../build/adstool}"

netid="$("${ads_tool}" "${remote}" netid)"
readonly netid

trap cleanup EXIT INT TERM
tmpfile="$(mktemp)"
readonly tmpfile

# always check state first to have the PLC in RUN!
check_state

check_raw

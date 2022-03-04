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

check_file() {
	local _test_string
	_test_string="$(date +%F%T)"
	readonly _test_string

	printf '%s\n' "${_test_string}" | check file write '%TC_INSTALLPATH%\test.txt'
	printf '%s\n' "${_test_string}" | check file write --append '%TC_INSTALLPATH%\test.txt'
	check file read '%TC_INSTALLPATH%\test.txt' > "${tmpfile}"
	printf '%s\n' "${_test_string}" "${_test_string}" | diff - "${tmpfile}"
	check file delete '%TC_INSTALLPATH%\test.txt'
	local _exit_code=0
	check file read '%TC_INSTALLPATH%\test.txt' || _exit_code=$?
	# return code is truncated to one byte so 1804 == 0x70C becomes 0x0C == 12
	if ! test 12 -eq "${_exit_code}"; then
		printf 'check_file() delete mismatch:\n>%s<\n>%s<\n' "12" "${_exit_code}" >&2
		return 1
	fi
}

check_license() {
	readonly EXPECTED_PLATFORMID="${EXPECTED_PLATFORMID-60}"
	local _platformid

	_platformid="$(check license platformid)"
	if ! test "${EXPECTED_PLATFORMID}" -eq "${_platformid}"; then
		printf 'check_license() platformid mismatch:\n>%s<\n>%s<\n' "${EXPECTED_PLATFORMID}" "${_platformid}" >&2
		return 1
	fi

	readonly EXPECTED_SYSTEMID="${EXPECTED_SYSTEMID-"12313354-E6B5-7F39-D952-04E011223BCC"}"
	local _systemid
	_systemid="$(check license systemid)"
	if ! test "${EXPECTED_SYSTEMID}" = "${_systemid}"; then
		printf 'check_license() systemid mismatch:\n>%s<\n>%s<\n' "${EXPECTED_SYSTEMID}" "${_systemid}" >&2
		return 1
	fi
}

check_loglevel() {
	local _hidden
	_hidden="$("${ads_tool}" [] --log-level=4 netid 2>&1 | tee "${tmpfile}" | wc -l)"
	if test "${_hidden}" -gt 0; then
		printf 'check_loglevel(): failed.\n--log-level=4 should have disable all messages, but we saw:\n' >&2
		cat "${tmpfile}"
		return 1
	fi

	local _default
	_default="$("${ads_tool}" [] netid 2>&1 | wc -l)"
	if ! test "${_default}" -gt 0; then
		printf 'check_loglevel() failed.\nBy default we should see an error message for our broken hostname\n' >&2
		return 1
	fi
}

check_pciscan() {
	check pciscan 0x15EC5000
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

rtime_max_latency() {
	local _latencies=

	if ! _latencies="$(check rtime read-latency)"; then
		return 1
	fi
	printf '%s' "${_latencies}" | awk '{print $3}' | sort | tail -n1 | wc -l
}

check_rtime() {
	local _old_latency
	local _new_latency

	_old_latency="$(rtime_max_latency)"
	_new_latency="$(rtime_max_latency)"
	if ! test "${_old_latency}" -le "${_new_latency}"; then
		printf 'check_rtime() latency mismatch:\n%s > %s\n' "${_old_latency}" "${_new_latency}" >&2
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

check_var() {
	local _response
	check var --type=BOOL 'MAIN.bInitTest' '1'
	_response="$(check var --type=BOOL 'MAIN.bInitTest')"
	if ! test "${_response}" = '1'; then
		printf 'check_var() BOOL mismatch:\n>%s<\n>%s<\n' "1" "${_response}" >&2
		return 1
	fi

	check var --type=BOOL 'MAIN.bInitTest' '0'
	_response="$(check var --type=BOOL 'MAIN.bInitTest')"
	if ! test "${_response}" = '0'; then
		printf 'check_var() BOOL mismatch:\n>%s<\n>%s<\n' "0" "${_response}" >&2
		return 1
	fi

	check var --type=DWORD 'MAIN.nTestCasesFailed' '10'
	_response="$( check var --type=DWORD 'MAIN.nTestCasesFailed')"
	if ! test "${_response}" = '10'; then
		printf 'check_var() DWORD mismatch:\n>%s<\n>%s<\n' "10" "${_response}" >&2
		return 1
	fi

	local _test_string
	_test_string="$(date +%F%T)"
	readonly _test_string
	check var --type=STRING 'MAIN.sLogPath' "${_test_string}"
	_response="$(check var --type=STRING 'MAIN.sLogPath')"
	if ! test "${_test_string}" = "${_response}"; then
		printf 'check_var() string mismatch:\n>%s<\n>%s<\n' "${_test_string}" "${_response}" >&2
		return 1
	fi
}

check_version() {
	local _debian_version
	local _our_version

	_our_version="$("${ads_tool}" --version)"
	_debian_version="$(awk '{print $2; exit}' "${script_path}/../debian/changelog")"
	if ! test "(${_our_version})" = "${_debian_version}"; then
		printf 'ERROR version missmatch between tool(%s) and debian package(%s)\n' \
			"${_our_version}" "${_debian_version}"
		return 1
	fi
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

# --log-level requires no PLC
check_loglevel

# always check state first to have the PLC in RUN!
check_state

check_file
check_license
check_pciscan
check_raw
check_rtime
check_var
check_version

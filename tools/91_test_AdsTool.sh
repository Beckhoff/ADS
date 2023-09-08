#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 - 2023 Beckhoff Automation GmbH & Co. KG
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
	${ads_tool} "${netid}" "--gw=${remote}" "$@"
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

check_file_find() {
	# Prepare a well defined directory tree on the test-device
	check file write '/dev/shm/adslib-test/1/a'
	check file write '/dev/shm/adslib-test/1/aa'
	check file write '/dev/shm/adslib-test/1/2/b'
	check file write '/dev/shm/adslib-test/1/2/3/c'
	check file write '/dev/shm/adslib-test/1/2/3/4/d'

	# List a full directory hierarchy
	cat >"${tmpfile}" <<- 'EOF'
		/dev/shm/adslib-test
		/dev/shm/adslib-test/1
		/dev/shm/adslib-test/1/2
		/dev/shm/adslib-test/1/2/3
		/dev/shm/adslib-test/1/2/3/4
		/dev/shm/adslib-test/1/2/3/4/d
		/dev/shm/adslib-test/1/2/3/c
		/dev/shm/adslib-test/1/2/b
		/dev/shm/adslib-test/1/a
		/dev/shm/adslib-test/1/aa
	EOF
	check file find "/dev/shm/adslib-test" | LC_ALL=C sort | diff --text - "${tmpfile}"

	# Limit max depth
	cat >"${tmpfile}" <<- 'EOF'
		/dev/shm/adslib-test
		/dev/shm/adslib-test/1
		/dev/shm/adslib-test/1/2
		/dev/shm/adslib-test/1/a
		/dev/shm/adslib-test/1/aa
	EOF
	check file find --maxdepth=2 "/dev/shm/adslib-test" | LC_ALL=C sort | diff --text - "${tmpfile}"

	# Single file or directory exists
	# Limit max depth
	cat >"${tmpfile}" <<- 'EOF'
		/dev/shm/adslib-test/1
	EOF
	check file find --maxdepth=1 "/dev/shm/adslib-test/1"
	cat >"${tmpfile}" <<- 'EOF'
		/dev/shm/adslib-test/1/a
	EOF
	check file find --maxdepth=1 "/dev/shm/adslib-test/1/a"

	# Non existing file
	if check file find "/dev/shm/adslib-test/missing"; then
		printf 'check_file_find(): succeeded where it should not!\n' >&2
		return 1
	fi
}

check_registry () {
	local _regfile="${script_path}/test-data/simple.reg"

	# Parse a registry file and dump our parsed output and compare it with the original
	check registry verify < "${_regfile}" | tee check-registry-verify.log | diff --text - "${_regfile}"
}

expected_id_pairs() {
	cat <<-EOF
		3F632E80-4FE8-2FCD-2B04-6154E399A473 90
		95EEFDE0-0392-1452-275F-1BF9ACCB924E 92
	EOF
}

check_license() {
	local _platformid
	if ! _platformid="$(check license platformid)"; then
		printf 'check_license() reading platformid failed\n' >&2
		return 1
	fi

	local _systemid
	if ! _systemid="$(check license systemid)"; then
		printf 'check_license() reading systemid failed\n' >&2
		return 1
	fi

	local _pair="${_systemid} ${_platformid}"
	if ! expected_id_pairs | grep --quiet "${_pair}"; then
		printf 'check_license() id mismatch:\n>%s<\nExpected one of these:\n' "${_pair}" >&2
		expected_id_pairs >&2
		return 1
	fi
}

check_loglevel() {
	local _hidden
	_hidden="$(${ads_tool} [] --log-level=4 netid 2>&1 | tee "${tmpfile}" | wc -l)"
	if test "${_hidden}" -gt 0; then
		printf 'check_loglevel(): failed.\n--log-level=4 should have disable all messages, but we saw:\n' >&2
		cat "${tmpfile}"
		return 1
	fi

	local _default
	_default="$(${ads_tool} [] netid 2>&1 | wc -l)"
	if ! test "${_default}" -gt 0; then
		printf 'check_loglevel() failed.\nBy default we should see an error message for our broken hostname\n' >&2
		return 1
	fi
}

check_pciscan() {
	check pciscan 0x15EC5000
}


check_plc_variable() {
	local _var="${1}"
	local _value="${2}"

	# Try to read a generic variable
	check plc read-symbol "${_var}" | tee "${tmpfile}.backup"

	# Overwrite a generic variable and verify the value was changed
	check plc write-symbol "${_var}" "${_value}"
	check plc read-symbol "${_var}" | diff - "${tmpfile}"

	# Now, restore the old value of the variable and verify the value
	# is back to original.
	check plc write-symbol "${_var}" "$(cat "${tmpfile}.backup")"
	check plc read-symbol "${_var}" | diff - "${tmpfile}.backup"
}

check_plc() {
	# Basic JSON output is working
	check plc show-symbols

	# Try generic access to integer variables
	printf '2147483647\n' > "${tmpfile}"
	check_plc_variable MAIN.nTestCasesFailed 2147483647
	check_plc_variable MAIN.nTestCasesFailed '0x7FFFFFFF'

	# BOOL and byte are more special cases we should verify
	printf '1\n' > "${tmpfile}"
	check_plc_variable MAIN.bInitTest 1
	printf '10\n' > "${tmpfile}"
	check_plc_variable MAIN.moreBytes 10

	# sLogPath is a T_MaxString, which is a 255 byte character array with one
	# additional byte for the zero terminator. So we expect 254 padding bytes
	# behind our string, when we read it back.
	{
		printf '10'
		dd if=/dev/zero bs=1 count=254
	} > "${tmpfile}"
	check_plc_variable MAIN.sLogPath '10'
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

check_startprocess() {
	local _result="result$(date +%s).sh"
	local _dir='/root'

	# Check that result cannot be executed yet (nonexistent)
	if check startprocess "${_dir}/${_result}"; then
		printf 'check_startprocess(): succeeded where it should not!\n' >&2
		return 1
	fi

	# Now, we use startprocess to create an empty executable on the target
	# device. We have to use /usr/bin/env, because the used tools have different
	# pathes on Debian and FreeBSD. Additionally, we wait a second after each
	# command, because ADS will return immediately after the process was started,
	# but we have no easy possibility to find out when it is finished. Yeah, we
	# could run a loop in /bin/sh but doing that correctly with a timeout limit
	# is not trivial and overkill, if sleep 1 is sufficient for our test!
	if ! check startprocess "--directory=${_dir}" /usr/bin/env "touch '${_result}'"; then
		printf 'check_startprocess(): failed to create empty file!\n' >&2
		return 1
	fi

	# Increase the chance that we lose the race against "touch" running on the target
	sleep 1
	if ! check startprocess --hidden /usr/bin/env "chmod +x '${_dir}/${_result}'"; then
		printf 'check_startprocess(): failed to make file executable!\n' >&2
		return 1
	fi

	# Increase the chance that we lose the race against "chmod" running on the target
	sleep 1
	# Check that we can now execute the result
	if ! check startprocess "${_dir}/${_result}"; then
		printf 'check_startprocess(): failed to run test file!\n' >&2
		return 1
	fi
}

check_state() {
	if ! check state; then
		printf 'check_state() could not read anything\n' >&2
		return 1
	fi

	# This might disconnect our ADS connection, so we ignore that expected error
	check state "${ADSSTATE_RESET}" || true

	# Now, wait until TwinCAT reaches RUN
	check "--retry=${BHF_CI_ADS_RETRY_LIMIT}" state --compare 5

	# And wait until the PLC is ready, too
	${ads_tool} "${netid}:851" "--gw=${remote}" "--retry=${BHF_CI_ADS_RETRY_LIMIT}" state --compare 5
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

	_our_version="$(${ads_tool} --version)"
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
readonly ads_tool="${QEMU_USER_EMULATION-} ${2-${script_path}/../build/adstool}"

netid="$(${ads_tool} "${remote}" netid)"
readonly netid

trap cleanup EXIT INT TERM
tmpfile="$(mktemp)"
readonly tmpfile

# --log-level requires no PLC
check_loglevel

# always check state first to have the PLC in RUN!
check_state

check_file
check_file_find
check_registry
check_license
check_pciscan
check_plc
check_raw
check_rtime
check_startprocess
check_var
check_version

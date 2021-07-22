#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2021 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

check() {
	"${ads_tool}" "${netid}" "--gw=${remote}" "$@"
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

check_state

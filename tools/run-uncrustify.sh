#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2023 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

run_uncrustify() {
	(
		cd "${script_path}/.."
		find Ads* example -name "*.h" -or -name "*.cpp" | uncrustify -c tools/uncrustify.cfg -F - "${@}"
	)
}


set -e
set -u

script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly script_path

case "${1-}" in
	check)
		run_uncrustify --check
		;;
	format)
		run_uncrustify --no-backup
		;;
	*)
		printf 'Unknown or missing command "%s"\n' "${1-}"
		printf 'Try "check" or "format"\n' "${1-}"
		exit 1
esac

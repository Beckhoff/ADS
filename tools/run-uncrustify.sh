#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2023 Beckhoff Automation GmbH & Co. KG
# Author: Patrick Bruenn <p.bruenn@beckhoff.com>

run_uncrustify() {
	(
		cd "${script_path}/.."
		find Ads* example \( -name "*.h" -or -name "*.cpp" \) -exec clang-format "${@}" {} \+
	)
}


set -e
set -u

script_path="$(cd "$(dirname "${0}")" && pwd)"
readonly script_path

case "${1-}" in
	check)
		run_uncrustify --dry-run --Werror
		;;
	format)
		run_uncrustify -i
		;;
	*)
		printf 'Unknown or missing command "%s"\n' "${1-}"
		printf 'Try "check" or "format"\n' "${1-}"
		exit 1
esac

#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) Beckhoff Automation GmbH & Co. KG

set -e
set -u

# Some flavor specific customizations
case "${CI_JOB_NAME}" in
	*alpine*)
		apk add g++ meson
		;;
	*arch*)
		pacman --noconfirm --refresh --sync --sysupgrade clang meson
		;;
	*mxe*)
		BHF_CI_MESON_OPTIONS='--cross-file meson.cross.amd64-linux.win32'
		;;
	*)
		;;
esac

meson setup build ${BHF_CI_MESON_OPTIONS-}
ninja -C build
meson setup example/build example ${BHF_CI_MESON_OPTIONS-}
ninja -C example/build

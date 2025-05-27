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
	*i686*)
		BHF_CI_MESON_OPTIONS='--cross-file meson.cross.amd64-linux.i686'
		;;
	*mips*)
		BHF_CI_MESON_OPTIONS='--cross-file meson.cross.amd64-linux.mips'
		export QEMU_LD_PREFIX='/usr/mips-linux-gnu'
		export QEMU_USER_EMULATION='qemu-mips'
		;;
	*mxe*)
		BHF_CI_MESON_OPTIONS='--cross-file meson.cross.amd64-linux.win32'
		;;
	*riscv64*)
		BHF_CI_MESON_OPTIONS='--cross-file meson.cross.amd64-linux.riscv64'
		export QEMU_LD_PREFIX='/usr/riscv64-linux-gnu'
		export QEMU_USER_EMULATION='qemu-riscv64'
		;;
	*tclur*):
		BHF_CI_MESON_OPTIONS='--native-file meson.native.tclur'
		;;
	*)
		;;
esac

meson setup build ${BHF_CI_MESON_OPTIONS-}
ninja -C build
meson setup example/build example ${BHF_CI_MESON_OPTIONS-}
ninja -C example/build

# If the job name contains 'test' we run tests, too.
case "${CI_JOB_NAME}" in
	*test*)
		./tools/90_run_tests.sh
		;;
	*)
		printf 'WARNING skipping tests for "%s".\n' "${CI_JOB_NAME}"
		;;
esac

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
	*tcbsd*):
		BHF_CI_MESON_OPTIONS='--native-file meson.native.tcbsd'
		SUDO_CMD=doas
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

# pkg-config makes cross-compiling a nightmare so we still stick to manual
# dependencies in our example/meson.build. However, to make native builds
# more easy we added pkg-config support. So here we go and explicitly test
# the "install" target, after we already build the "example".
${SUDO_CMD-} ninja -C build install

# We need the AdsTest tool in different flavours for our test pack. The test
# runner always runs on Linux, but it can either be amd64 or arm64.
case "${CI_JOB_NAME}" in
	*tclur*)
		readonly adstest_dir='.test-pack/T_100_test_delete_reg_key/bin'
		mkdir -p "${adstest_dir}"
		cp -a "build/AdsTest${ADSTEST_FILE_EXTENSION-}" "${adstest_dir}/AdsTest-$(uname -m)"
		;;
	*)
		# We don't need other platforms than tclur amd64/arm64.
		;;
esac

# If the job name contains 'test' we run tests, too.
case "${CI_JOB_NAME}" in
	*test*)
		./tools/90_run_tests.sh
		;;
	*)
		printf 'WARNING skipping tests for "%s".\n' "${CI_JOB_NAME}"
		;;
esac

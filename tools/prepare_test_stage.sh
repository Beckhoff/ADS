#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

download_iso() {
	luemuctl get-latest-tcbsd

	# Noninteractive doas is a critical requirement in CI. We need it to
	# gather crashdumps and other error diagnostics, when a test breaks.
	# We learned the hard way, that it is a very bad idea to try to enable
	# noninteractive doas on demand in such a situation.
	# Instead we make it available as soon as possible, as a post-install.d trigger.
	guestfish --add TCBSD*.iso -m /dev/sda1:/ upload - /INSTALLER/post_install.d/999-noninteractive-doas.chroot <<- EOF
		#!/bin/sh
		set -e
		set -u

		cp -a /usr/local/etc/doas.conf /usr/local/etc/doas.conf.before-ci
		printf 'permit nopass Administrator\n' > /usr/local/etc/doas.conf
	EOF
}

set -e
set -u

download_iso
jitlab shallow-clone test_stage "${BHF_CI_TEST_STAGE_GIT_REF:-master}"

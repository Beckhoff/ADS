#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 - 2021 Beckhoff Automation GmbH & Co. KG

set -e
set -u

if test -z "${SSH_AUTH_SOCK-}"; then
	eval $(ssh-agent -s)
	ssh-add - < "${SSH_KEY_APTLY}"
fi

script_path="$(cd "$(dirname "${0}")" && pwd)"
bdpg git-pre-push-check --package-dir="${script_path}/../" master
bdpg push "${script_path}/../debian-release/"*.deb

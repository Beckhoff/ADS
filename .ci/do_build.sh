#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

set -e
set -u

readonly script_path="$(cd "$(dirname "$0")" && pwd)"
readonly make_cmd="${MAKE-make} -j$(nproc)"
export OS_NAME=${OS_NAME-$(uname)}
export CXX=${CXX-g++}

${make_cmd} -C "${script_path}/../"
${make_cmd} -C "${script_path}/../" AdsLibTest.bin
${make_cmd} -C "${script_path}/../" AdsLibOOITest.bin
${make_cmd} -C "${script_path}/../example/"

# collect build artifacts into a single folder to make azdevops easier...
readonly artifacts="${script_path}/../bin/${OS_NAME}/${CXX##*/}"
mkdir -p "${artifacts}"
mv "${script_path}/../AdsLib"*.a "${artifacts}/"
mv "${script_path}/../AdsLib"*.bin "${artifacts}/"
mv "${script_path}/../example/example.bin" "${artifacts}/"

#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) Beckhoff Automation GmbH & Co. KG

set -e
set -u

meson setup build ${BHF_CI_MESON_OPTIONS-}
ninja -C build
meson setup example/build example ${BHF_CI_MESON_OPTIONS-}
ninja -C example/build

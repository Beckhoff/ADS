// SPDX-License-Identifier: MIT
/**
    Copyright (C) 2023 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#endif

namespace bhf
{
static inline void ForceBinaryOutputOnWindows()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	(void)_setmode(_fileno(stdout), O_BINARY);
#endif
}
}

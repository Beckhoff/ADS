REM SPDX-License-Identifier: MIT
REM Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

REM prepare environment variables
call "C:\BuildTools\Common7\Tools\VsDevCmd.bat"
call "C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

msbuild AdsLib.sln /property:Configuration=Debug /property:Platform=win32
msbuild AdsLib.sln /property:Configuration=Release /property:Platform=win32

REM collect build artifacts into a single folder to make azdevops easier...
mkdir bin\win32
move Debug bin\win32
move Release bin\win32

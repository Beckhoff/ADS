#!/bin/bash

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=fructose-1.3.0

pushd ${SCRIPT_PATH}

if ! sha512sum -c ${NAME}.sha512; then
	wget https://downloads.sourceforge.net/project/fructose/fructose/${NAME}/${NAME}.tgz
fi

tar xf ${NAME}.tgz
mv ${NAME}/fructose/include/fructose/ ${SCRIPT_PATH}/
rm -rf ${NAME}/

patch fructose/test_root.h 0001-fructose-test_root-split-use-size_t-for-start-end.patch
patch fructose/test_root.h 0002-fructose-use-int-for-i-iterating-over-argc.patch

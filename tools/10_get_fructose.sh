#!/bin/sh

SCRIPT_PATH="$( cd "$( dirname "$0" )" && pwd )"
NAME=fructose-1.4.0
FILENAME=${NAME}.tar.gz

cd ${SCRIPT_PATH}

if ! sha512sum -c ${NAME}.sha512; then
	wget https://downloads.sourceforge.net/project/fructose/fructose/${NAME}/${FILENAME}
fi

tar xf ${FILENAME}
rm -rf ${SCRIPT_PATH}/fructose
mv ${NAME}/fructose/include/fructose/ ${SCRIPT_PATH}/
rm -rf ${NAME}/

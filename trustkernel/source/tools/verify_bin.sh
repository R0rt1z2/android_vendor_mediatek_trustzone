#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $(basename $0) <binaries>"
	exit 255
fi

bin=$1

if [ ! -f ${bin} ]; then
	echo "File ${bin} not exist"
	exit 255
fi

strings ${bin} | egrep "tkcore_.*_commit-.*"  > /dev/null

if [ $? -ne 0 ]; then
	echo "Invalid binary. Binary not compiled from valid repo"
	exit 255
fi

echo "Verifying ${bin} ... PASSED"

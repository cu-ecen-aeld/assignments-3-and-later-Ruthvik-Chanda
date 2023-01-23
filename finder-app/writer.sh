#!/bin/bash

writefile=$1
writestr=$2

if [ $# -ne 2 ]
then
	echo "Error"
exit 1
fi

full_directory=` dirname ${writefile}`
if [ ! -d "$full_directory" ]
then
	mkdir -p ${full_directory}
fi

if touch ${writefile}
then
	echo ${writestr} > ${writefile}
else
	echo "File dint get created"
exit 1
fi

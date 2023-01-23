#!/bin/bash

filesdir=$1
searchstr=$2

if [ $# -ne 2 ]
then
	echo "Error"
exit 1
fi

if [ -d "$filesdir" ]
then
	echo "File created"
	files=` find $filesdir -type f | wc -l`
	lines=`grep -r ${searchstr} ${filesdir} | wc -l`
	echo "The number of files are $files and the number of matching lines are $lines"
exit 0
else
	echo "File is not created"
exit 1
fi

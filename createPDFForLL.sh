#!/bin/sh

FILENAME=$2

case "$1" in
	'-cfg')	
		OPT_PASSES="-dot-cfg"
		;;
	'-callgraph')
		OPT_PASSES="-dot-callgraph"
		;;
	'-dom')
		OPT_PASSES="-dot-dom"
		;;
	*)
		echo "invalid argument,"
		echo "Usage: runPass	[ -cfg | -callgraph | -dom ] <LL filename>"
		exit 1
		;;
esac

if [[ $# != 2 ]]
then
	echo "invalid number of arguments,"
	echo "Usage: runPass	[ -cfg | -callgraph | -dom ] <LL filename>"
	exit 1
fi

opt $OPT_PASSES < $FILENAME 2> /dev/null

for dotFile in *.dot
do
	echo "-> generating pdf for ${dotFile%.dot}.pdf"
	dot -Tpdf -O $dotFile
	rm $dotFile
done

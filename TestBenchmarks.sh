#!/bin/sh


shopt -s dotglob
shopt -s nullglob

getMakeOption(){
	#echo "----"
	#cat "${1}Makefile"
	#echo "----"
	optionLine=`grep ^${2} "${1}Makefile"`
	if [[ $optionLine != "" ]]
	then
		echo "foundOption"
		echo $optionLine
	else
		echo "noOptionFound"
		echo $optionLine
	fi
}

compileBenchmark(){
	echo "--compiling benchmark ${1}"
	getMakeOption ${1} "CPPFLAGS"
	getMakeOption ${1} "PROG"
	echo "\n\n"
}

runBenchmark () {
	echo "run benchmark"
}

cd benchmarks
BenchmarkFolders=(*/)

for BenchmarkFolder in ${BenchmarkFolders[@]}
do
	echo $BenchmarkFolder
	compileBenchmark $BenchmarkFolder
done








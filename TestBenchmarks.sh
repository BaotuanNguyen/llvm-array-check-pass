#!/bin/sh
ccWOpts="-Wcomment -Wstring-plus-int -Wformat -S -emit-llvm"
LibFile="../LibArrayCheck.c"
LibFileLL="${LibFile%.c}.ll"
LLVM_LIBRARY=../../../Release+Asserts/
cd benchmarks/
MODULE_LIB=`ls ${LLVM_LIBRARY}lib/llvm-array-check-pass*`
cd ../
OPT="./${LLVM_LIBRARY}bin/opt"
PASSES="-loop-unroll"

shopt -s dotglob
shopt -s nullglob


getMakeOption(){
	#echo "----"
	#cat "${1}Makefile"
	#echo "----"
	optionLine=`grep ^${2} "${1}Makefile"`
	option=""
	if [[ $optionLine != "" ]]
	then
		option="$(echo $optionLine | sed -n 's/[^=]*=//p' )"
		echo $option
	fi
}

compileBenchmark(){
	echo "--compiling benchmark ${1}"
	cat "${1}Makefile"
	#echo "--"
	echo "\n"
	ccOpts=$(getMakeOption ${1} "CPPFLAGS")
	progOpt=$(getMakeOption ${1} "PROG")
	ldOpts=$(getMakeOption ${1} "LDFLAGS")
	cFiles=(${1}*.c)
	#oFiles=""
	llModFiles=""
	#compile all benchmarks
	for cFile in ${cFiles[@]}
	do
		oFile="${cFile%.c}.o"
		llFile="${cFile%.c}.ll"
		llModFile=${cFile%.c}".mod.ll"
		#oFiles=$oFiles" "$oFile
		llModFiles=$llModFiles" "$llModFile
		#echo "-> compiling ${cFile} into ${llFile} with in (${1}) options (${ccOpts} <> ${ccWOpts})"
		echo "-> compiling ${cFile} into ${llFile}"
		#emitting intermediate llvm IR code
		clang ${ccOpts} -I${1} ${ccWOpts} -o ${llFile} ${cFile} 2> /dev/null
		#echo "clang ${ccOpts} -I${1} ${ccWOpts} -o ${llFile} ${cFile} 2> /dev/null"
		#optimize intermediate llvm code
		echo "-> optimizing ${llFile} into ${llModFile} with (${PASSES})"
		#echo "$OPT -load ${MODULE_LIB} $PASSES -S -o $llModFile < $llFile > /dev/null"
		$OPT -load ${MODULE_LIB} $PASSES -S -o $llModFile < $llFile > /dev/null
	done
	#echo "-> linking ${progOpt} ld-options (${ldOpts}) object-files (${oFiles})"
	echo "-> linking ${progOpt} ld-options (${ldOpts}) object-files (${llModFiles})"
	#link all benchmarks 
	clang -o ${1}${progOpt} ${llModFiles} ${ldOpts} -lstdc++
	echo "--"
	echo "cpp flags : ${cppOpts}"
	echo "program   : ${progOpt}"
	echo "ld flags  : ${ldOpts}"
	echo "\n\n"
}

runBenchmark () {
	progOpt=$(getMakeOption ${1} "PROG")
	runOpt=$(getMakeOption ${1} "RUN_OPTIONS")
	prog="./${1}${progOpt}"
	sedDir=${1%\/}
	sedOpts="s/\$(PROJ_SRC_DIR)/"$sedDir"/gp"
	runOpts=$(echo $runOpt | sed -n $sedOpts)
	echo "\n\n--"
	echo "->running ${prog} ${runOpts}"
	#run the application to obtain any output
	$prog $runOpts &> "$sedDir.txt"
	#run the application for timing
	#timeOutput=`/usr/bin/time $prog $runOpts 2>&1 >/dev/null`
	timeOutput=$((time $prog $runOpts >/dev/null) 2>&1 | awk '/[0-9]*\.[0-9]*/{print;}' )
	#timeExec=$( echo $timeOutput | awk '/^user/ { print "$0" }' )
	echo "*"
	echo "$timeOutput"
	echo "*"
	#remove executable
	rm ${1}${progOpt}
}



#MAIN() {
#setup library file

#remove Output folder, output.txt, and stat.txt (contains all important statistics
TIMEFORMAT=%R

cd benchmarks/
rm -rf Output/
rm -rf *.txt
rm -rf *.stat.txt
cd ../

#compile library file
clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o ${LibFileLL} ${LibFile}
clang -emit-llvm -c -o ${LibFileLL} ${LibFile}
echo "clang -emit-llvm -c -o ${LibFileLL} ${LibFile}"

#get a lof of all the benchmarks
cd benchmarks
BenchmarkFolders=(*/)

#compile every benchmark
for BenchmarkFolder in ${BenchmarkFolders[@]}
do
	echo $BenchmarkFolder
	compileBenchmark $BenchmarkFolder
done

#create benchmark output folder
mkdir Output

#run every benchmark
pwd
for BenchmarkFolder in ${BenchmarkFolders[@]}
do
	echo $BenchmarkFolder
	runBenchmark $BenchmarkFolder
done

#main end ]

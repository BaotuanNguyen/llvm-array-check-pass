#!/bin/sh
ccWOpts="-w -S -emit-llvm"
LibFile="../LibArrayCheck.cpp"
LibFileLL="${LibFile%.cpp}.ll"
LLVM_LIBRARY=../../../Release+Asserts/
cd benchmarks/
MODULE_LIB=`ls ${LLVM_LIBRARY}lib/llvm-array-check-pass*`
cd ../
OPT="./${LLVM_LIBRARY}bin/opt"
PASSES="-loop-unroll"

shopt -s dotglob
shopt -s nullglob


PASSES=""	#no optimizations
VERBOSE=0

usage() {
	usageMsg="usage TestBenchmarks [options]
options
	-1	unoptimized checks inserted
	-2	optimized using global check removal
	-3 	optimized using loop check removal
	-v	verbose output
	-h	help"
	echo "$usageMsg"
}


#parse the options
while getopts "123avh" OPTION
do
	case "$OPTION" in
		"1")	PASSES="-array-check"
			;;
		"2")	PASSES="-remove-global"
			;;
		"3") 	PASSES="-loop-pass"
			;;
		"a")	;;
		"v")	VERBOSE=1
			;;
		"h")	usage
			exit 1
			;;
		"?")	
			usage
			exit 1
			;;
esac
done

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

#compileBenchmark
#	$1 	folder name
#	$2 	optimization pass options
compileBenchmark(){
	#echo "--compiling benchmark ${1}"
	#cat "${1}Makefile"
	#echo "--"
	#echo "\n"
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

		#emitting intermediate llvm IR code
		clang ${ccOpts} -I${1} ${ccWOpts} -o ${llFile} ${cFile}
		#echo "clang ${ccOpts} -I${1} ${ccWOpts} -o ${llFile} ${cFile} 2> /dev/null"
		#echo "-> compiling ${cFile} into ${llFile} with in (${1}) options (${ccOpts} <> ${ccWOpts})"
		#echo "-> compiling ${cFile} into ${llFile}"

		#collect statistics of optimizations here
		$OPT -load ${MODULE_LIB} $2 -S -o $llModFile < $llFile > /dev/null
		#echo "-> optimizing ${llFile} into ${llModFile} with (${2})"
		#echo "$OPT -load ${MODULE_LIB} $PASSES -S -o $llModFile < $llFile > /dev/null"
	done
	#echo "-> linking ${progOpt} ld-options (${ldOpts}) object-files (${oFiles})"
	#echo "-> linking ${progOpt} ld-options (${ldOpts}) object-files (${llModFiles})"
	#link all benchmarks 
	clang -o ${1}${progOpt} ${LibFileLL} ${llModFiles} ${ldOpts} -lstdc++
	#echo "--"
	#echo "cpp flags : ${cppOpts}"
	#echo "program   : ${progOpt}"
	#echo "ld flags  : ${ldOpts}"
	#echo "\n\n"
}

#inputs are
#$1	program folder
#$2	program name
#$3 	program options
runBenchmark () {
	progOpt="${2}"
	runOpt="${3}"
	#program executable with path
	prog="./${1}${progOpt}"
	#subtitute in program program options
	sedDir=${1%\/}
	sedOpts="s/\$(PROJ_SRC_DIR)/"$sedDir"/gp"
	runOpts=$(echo $runOpt | sed -n $sedOpts)
	#echo "\n\n--"
	#echo "->running ${prog} ${runOpts}"
	#run the application to obtain any output
	$prog $runOpts &> "$sedDir.txt"
	#run the application for timing
	#timeOutput=`/usr/bin/time $prog $runOpts 2>&1 >/dev/null`
	timeOutput=$((time $prog $runOpts >/dev/null) 2>&1 | awk '/[0-9]*\.[0-9]*/{print;}' )
	#timeExec=$( echo $timeOutput | awk '/^user/ { print "$0" }' )
	execSize=$( wc -c $prog | awk '{print $1;}' )
	echo "RunTime  (secs) : ${timeOutput}"
	echo "ExecSize (bytes): ${execSize}"
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
cd benchmarks/
echo "clang++ -D__DEBUG__=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o ${LibFileLL} ${LibFile}"
clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o ${LibFileLL} ${LibFile}
cd ../

#get a lof of all the benchmarks
cd benchmarks
BenchmarkFolders=(*/)

#create benchmark output folder
mkdir Output

#compile every benchmark
for BenchmarkFolder in ${BenchmarkFolders[@]}
do
	echo $BenchmarkFolder
	compileBenchmark "$BenchmarkFolder" "$PASSES"
	echo "compiled and linked \"${BenchmarkFolder%/}\" with optimizations (${PASSES})"

	progOpt=$(getMakeOption ${BenchmarkFolder} "PROG")
	runOpt=$(getMakeOption ${BenchmarkFolder} "RUN_OPTIONS")
	#run the actualy benchmark
	echo "running ${BenchmarkFolder%/}"
	runBenchmark "$BenchmarkFolder" "$progOpt" "$runOpt"
	echo "\n\n"
done

#main end ]

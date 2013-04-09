ReportFile="REPORT.txt"
ReportFileOut="REPORT.out.txt"
IFS="
"

rm $ReportFileOut

for fileLine in $( cat $ReportFile )
do
	#echo "echo $fileLine | egrep \"(\ |\\t)*\$\$[1-4]*\""
	echo $fileLine | egrep '(\ |\\t)*\$\$[0-2]' > /dev/null
	if [[ $? -eq "0" ]]; then
		#replace line with text from test
		echo "->${fileLine}"
		bench=$( echo $fileLine | sed -n 's/\(\ |\t\)*$$\([0-2]\)/\2/gp' | bc )
		echo "->$bench"
		echo "cat \"output${bench}.txt\" >> $ReportFileOut"
		cat "output${bench}.txt" >> $ReportFileOut
#		case $bench in
#			"1")cat output0;;
#			"2");;
#			"3");;
#			"4");;
#		esac
	else
		echo "${fileLine}" >> $ReportFileOut
		#echo "${fileLine}"
	fi
done

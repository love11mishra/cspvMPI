#!/bin/bash

start=$(date "+%s")

Nproc=$1						# the limit number of processes
total_mission=216			# the number of missions
file_name=./para_list



trap "exec 1000<&-; exec 1000>&-; exit 0" 2
rm -fr testfifo
mkfifo testfifo
exec 1000<>testfifo
rm -fr testfifo

for ((n=1; n<=$Nproc; n++))
do
	echo >&1000
done


mission_i=0
cat $file_name | while read line;
do 
  read -u1000
  {
	  # ../reproduce.sh DTG 0 5 3600 0
	  ../reproduce.sh $line;
	  echo >&1000
	  mission_i=`expr $mission_i + 1`
	  if [ $mission_i -eq $total_mission ]; then
		break
	  fi
  } &
done


wait
exec 1000<&-
exec 1000>&-


../reproduce.sh DepSolver 0 6 3600 0
../reproduce.sh DepSolver 0 8 3600 0
../reproduce.sh DepSolver 0 10 3600 0

../reproduce.sh DepSolver 0 6 3600 1
../reproduce.sh DepSolver 0 8 3600 1
../reproduce.sh DepSolver 0 10 3600 1


end=$(date "+%s")

echo "TIME ELAPSED: $(expr $(expr ${end} - ${start}) / 60) minutes." | tee ElapsedTime
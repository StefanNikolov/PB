#!/bin/bash

if [[ $1 = "" ]]; then
    echo "Please use \"make test\" to run the tests."
    exit
fi


logfile=TESTRESULTS.log
nettime=0

input=( apbs-mol apbs-smol apbs-spl2 ) 

acetic=( -2.267873880748E+01 -2.233042654941E+01 -3.579918772262E+01 ) 
acetate=( -1.997461573276E+02 -1.984881352623E+02 -2.317244846973E+02 ) 
proton=( -2.974598683157E+02 -2.959669997009E+02 -3.670680616644E+02 )
ionization=( -4.745272868358E+02 -4.721247084138E+02 -5.629933586391E+02 )

# Initialize the results file

date=`date`
echo "Date     : ${date}" >> $logfile
echo "Directory: ionize" >> $logfile
echo "Results  :" >> $logfile


# For each file in the directory, run APBS and get the value

for i in 0 1 2 
do
  echo "----------------------------------------"
  echo "Testing input file ${input[i]}.in"
  echo ""

 
  starttime=`date +%s`
  $1 ${input[i]}.in > ${input[i]}.out 
  answer=( `grep "Global net" ${input[i]}.out | awk '{print $5}'` )
  sync

  # Acetic Acid

  echo "Acetic Acid Energy: ${answer[0]}"
  if [[ ${answer[0]} == ${acetic[i]} ]]; then
      echo "*** PASSED ***"
      echo "           ${input[i]}.in (Acetic Acid): PASSED (${answer[0]})" >> $logfile
  else
      echo "*** FAILED ***"
      echo "   APBS returned ${answer[0]}"
      echo "   Expected result is ${acetic[i]}"
      echo "           ${input[i]}.in (Acetic Acid): FAILED (${answer[0]}; expected ${acetic[i]})" >> $logfile
  fi

  # Acetate

  echo "Acetate Energy    : ${answer[1]}"
  if [[ ${answer[1]} == ${acetate[i]} ]]; then
      echo "*** PASSED ***"
      echo "           ${input[i]}.in (Acetate): PASSED (${answer[1]})" >> $logfile
  else
      echo "*** FAILED ***"
      echo "   APBS returned ${answer[1]}"
      echo "   Expected result is ${acetate[i]}"
      echo "           ${input[i]}.in (Acetate): FAILED (${answer[1]}; expected ${acetate[i]})" >> $logfile
  fi

  # Proton

  echo "Proton Energy     : ${answer[2]}"
  if [[ ${answer[2]} == ${proton[i]} ]]; then
      echo "*** PASSED ***"
      echo "           ${input[i]}.in (Proton): PASSED (${answer[2]})" >> $logfile
  else
      echo "*** FAILED ***"
      echo "   APBS returned ${answer[2]}"
      echo "   Expected result is ${proton[i]}"
      echo "           ${input[i]}.in (Proton): FAILED (${answer[2]}; expected ${proton[i]})" >> $logfile
  fi

  # Ionization

  echo "Ionization Energy : ${answer[3]}"
  if [[ ${answer[3]} == ${ionization[i]} ]]; then
      echo "*** PASSED ***"
      echo "           ${input[i]}.in (Ionization): PASSED (${answer[3]})" >> $logfile
  else
      echo "*** FAILED ***"
      echo "   APBS returned ${answer[3]}"
      echo "   Expected result is ${ionization[i]}"
      echo "           ${input[i]}.in (Ionization): FAILED (${answer[3]}; expected ${ionization[i]})" >> $logfile
  fi

  endtime=`date +%s`
  let elapsed=$endtime-$starttime
  let nettime=$nettime+$elapsed
  echo "Total elapsed time: $elapsed seconds"
  echo "----------------------------------------"


done

echo "Test results have been logged to ${logfile}."
echo "Total time for this directory: ${nettime} seconds."

echo "Time     : ${nettime} seconds" >> $logfile
echo "" >> $logfile
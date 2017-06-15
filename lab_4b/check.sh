#!/bin/bash

# # # # # # # # # # #
# Nathan Tsai       #
# nwtsai@gmail.com  #
# 304575323         #
# # # # # # # # # # #

# MAKE CHECK TEST SCRIPT

# NORMAL CASE
./lab4b &> /dev/null <<-EOF
OFF 
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "Error: program did not run correctly in the normal case" >> errors.txt
fi

# NON-NUMERICAL PERIOD LONG OPTION ARGUMENT
./lab4b --period=a123 &> /dev/null
ret=$?
if [ $ret -ne 1 ] 
then
	echo "Error: program did not catch an initial invalid period argument" >> errors.txt
fi

# NEGATIVE PERIOD LONG OPTION ARGUMENT
./lab4b --period=-123 &> /dev/null
ret=$?
if [ $ret -ne 1 ] 
then
	echo "Error: program did not catch an initial negative period argument" >> errors.txt
fi

# INVALID SCALE LONG OPTION ARGUMENT
./lab4b --scale=G &> /dev/null
ret=$?
if [ $ret -ne 1 ]
then
	echo "Error: program did not catch an invalid scale argument" >> errors.txt
fi

# CHECK IF A LOGFILE IS SUCCESSFULLY CREATED
./lab4b --log=logfile.txt &> /dev/null <<-EOF
OFF 
EOF
if [ ! -f logfile.txt ] 
then 
	echo "Error: program could not create and open a logfile" >> errors.txt
fi
rm -f logfile.txt

# UNRECOGNIZED LONG OPTION
./lab4b --bogus &> /dev/null
ret=$?
if [ $ret -ne 1 ] 
then
	echo "Error: program did not catch an initial unrecognized long option" >> errors.txt
fi

# UNRECOGNIZED STDIN COMMAND
./lab4b &> /dev/null <<-EOF
BOGUS
EOF
ret=$?
if [ $ret -ne 1 ] 
then 
	echo "Error: program could not catch an invalid STDIN command" >> errors.txt
fi

# NON-NUMERICAL PERIOD STDIN COMMAND
./lab4b --period=5 &> /dev/null <<-EOF
PERIOD=abcd
EOF
ret=$?
if [ $ret -ne 1 ] 
then
	echo "Error: program could not catch a non-numerical period value from STDIN" >> errors.txt
fi

# NEGATIVE PERIOD STDIN COMMAND
./lab4b --period=5 &> /dev/null <<-EOF
PERIOD=-123
EOF
ret=$?
if [ $ret -ne 1 ] 
then
	echo "Error: program could not catch a negative period value from STDIN" >> errors.txt
fi

# CHECK IF ALL OF THE ABOVE TEST CASES PASSED
if [ -a errors.txt ]
then
	echo "Failed the following test cases:"
	cat errors.txt
	rm -f errors.txt
else
	echo "Passed all test cases"
fi 
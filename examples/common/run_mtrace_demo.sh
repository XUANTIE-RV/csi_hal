#!/bin/sh

export MALLOC_TRACE=/tmp/mtrace_demo_mtrace.log
rm -f $MALLOC_TRACE
# set '/proc/sys/kernel/randomize_va_space' to be 0
# to disable Address Space Layout Randomization(ASLR)
#sudo sysctl -w kernel.randomize_va_space=0

echo  "\n**** Run mtrace demo ****"
./mtrace_demo

echo  "\n**** mtrace analysis result ****"
mtrace ./mtrace_demo $MALLOC_TRACE | grep -E "\.c|\.cpp"

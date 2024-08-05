#!/bin/sh
while :
do
current_date_time="`date "+%Y-%m-%d %H:%M:%S"`"
MemAvailable="`cat /proc/meminfo | grep -i "memav"|awk '{print $2}'`"
MemFree="`cat /proc/meminfo | grep -i "memfree"|awk '{print $2}'`"
Process="`top -m -n 1| grep -i "Process"`"
Process_CPU="`top -n 1 | grep "Process" | awk '{print $7}'`"
Process_mem=`cat /sys/fs/cgroup/process/memory.current`

idle="`busybox top -n 1 | grep idle | awk '{print $8}'`"
echo -n $current_date_time
echo -n "  MemAvailable= "
echo -n $MemAvailable
echo -n "  MemFree= "
echo -n $MemFree
echo -n "  Process_Mem= "
echo -n $Process_mem
echo -n "  Process_mem= "
echo -n | awk -v "sp=$Process" 'BEGIN {split (sp,array);printf array[4]}'
echo -n "  Process_PID= "
echo -n | awk -v "sp=$Process" 'BEGIN {split (sp,array);printf array[1]}'
echo -n "  CPU_idle= "
echo -n $idle
echo -n "  Process_CPU= "
echo $Process_CPU
sleep 5
done

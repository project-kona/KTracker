#!/bin/bash
N=`nproc`
for ((cpu=0; cpu<$N; cpu++)); do
  G=/sys/devices/system/cpu/cpu$cpu/cpufreq/scaling_governor
  [ -f $G ] && echo performance > $G
done


#!/bin/bash

source ./run_make_pbsim.sh

TURI_HOME='../../apps/turi'

RESDIR=$1/turi

NUMACMD="numactl -N 0 -m 0"

cleanup() {
  echo "Killing old processes..."
  sudo pkill -2 tracker
  sleep 5 
  sudo pkill -9 python3 
  sleep 1
  sudo rm -f dcl_*.bin res_pbsim_*.txt 
}

run_turi() {
  cleanup
  local APP=$1
  local GRAPH=$2
  echo "Running Turi" $APP $GRAPH $(date +%Y-%m-%d_%H-%M-%S)
  $NUMACMD python3 $TURI_HOME/app_graph_analytics.py -a $APP -g $GRAPH  > $RESDIR/output_turi_${APP}_${GRAPH}_orig.txt
  cleanup
}

run_turi_pbsim() {
  cleanup
  local APP=$1
  local GRAPH=$2
  echo "Running Turi + PBSim" $APP $GRAPH $(date +%Y-%m-%d_%H-%M-%S)
  $NUMACMD python3 $TURI_HOME/app_graph_analytics.py -a $APP -g $GRAPH -s > $RESDIR/output_turi_${APP}_${GRAPH}_pbsim_$3.txt
  sleep 5
  sudo chown $USER: dcl_*.bin
  sudo chown $USER: res_pbsim_*.txt
  sudo mv dcl_*.bin $RESDIR/turi_${APP}_${GRAPH}_dcl_$3.bin
  sudo mv res_pbsim_*.txt $RESDIR/res_pbsim_turi_${APP}_${GRAPH}_$3.txt
  cleanup
}


mkdir -p $RESDIR

########### PBSIM ##############

CMD=(cl wp report_times)

for cmd in ${CMD[@]}; do
  make_pbsim_$cmd
  cp $PBSIM_HOME/tracker tracker
  
  run_turi_pbsim pagerank twitter $cmd
  run_turi_pbsim connectedcomp twitter $cmd
  run_turi_pbsim labelprop twitter $cmd
  run_turi_pbsim graphcol twitter $cmd
  rm -f tracker
done


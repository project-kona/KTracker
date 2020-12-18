#!/bin/bash

source ./run_make_pbsim.sh

THR=1
TRACKER=$PBSIM_HOME/tracker

RESDIR=$1/metis

LR_INPUT_SMALL=lr_4GB.txt
LR_INPUT_LARGE=lr_40GB.txt
HIST_INPUT_SMALL=hist-2.6g.bmp
HIST_INPUT_LARGE=hist-40g.bmp

#LR_INPUT=$LR_INPUT_SMALL
#HIST_INPUT=$HIST_INPUT_SMALL

LR_INPUT=$LR_INPUT_LARGE
HIST_INPUT=$HIST_INPUT_LARGE

NUMACMD="numactl -N 0 -m 0"

cleanup() {
  echo "Killing old processes..."
  sudo pkill -2 tracker 
  sleep 2 
  sudo pkill -9 hist
  sudo pkill -9 linear
  sudo pkill -9 -f run_writeprotect.sh
  sudo rm -f dcl_*.bin
  sudo rm -f res_pbsim_*.txt
  sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
  sleep 2
}

run_pbsim_lr() {
  cleanup
  local cmd="$NUMACMD sudo $TRACKER -c ~/Metis/obj/linear_regression ~/Metis/data_tool/data/$LR_INPUT -p $THR > $RESDIR/output_metis_lr_pbsim_${THR}_$1_${LR_INPUT}.txt"
  echo $cmd
  eval "$cmd"
  sudo chown icalciu: dcl_*.bin 
  sudo chown icalciu: res_pbsim_*.txt
  sudo mv dcl_*.bin $RESDIR/lr_dcl_${THR}_$1_${LR_INPUT}.bin
  sudo mv res_pbsim_*.txt $RESDIR/res_pbsim_lr_${THR}_$1_${LR_INPUT}.txt
  cleanup
}

run_pbsim_hist() {
  cleanup 
  local cmd="$NUMACMD sudo $TRACKER -c ~/Metis/obj/hist ~/Metis/data_tool/data/$HIST_INPUT -q -p $THR > $RESDIR/output_metis_hist_pbsim_${THR}_$1_${HIST_INPUT}.txt" 
  echo $cmd
  eval "$cmd"
  sudo chown icalciu: dcl_*.bin 
  sudo chown icalciu: res_pbsim_*.txt
  sudo mv dcl_*.bin $RESDIR/hist_dcl_${THR}_$1_${HIST_INPUT}.bin
  sudo mv res_pbsim_*.txt $RESDIR/res_pbsim_hist_${THR}_$1_${HIST_INPUT}.txt
  cleanup
}

run_lr() {
  cleanup
  local cmd="$NUMACMD ~/Metis/obj/linear_regression ~/Metis/data_tool/data/$LR_INPUT  -p $THR > $RESDIR/output_metis_lr_orig_$THR_${LR_INPUT}.txt"
  echo $cmd
  eval "$cmd"
  cleanup
}

run_hist() {
  cleanup
  local cmd="$NUMACMD ~/Metis/obj/hist ~/Metis/data_tool/data/$HIST_INPUT -q -p $THR > $RESDIR/output_metis_hist_orig_$THR_${HIST_INPUT}.txt" 
  echo $cmd
  eval "$cmd"
  cleanup
}

mkdir -p $RESDIR

######### PBSIM ##################

CMD=(cl wp report_times)

for cmd in ${CMD[@]}; do
  make_pbsim_$cmd

  run_pbsim_hist $cmd
  run_pbsim_lr $cmd
done

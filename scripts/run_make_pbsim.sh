#!/bin/bash

AUTO="### This file is generated automatically. Any changes will be lost. Please see Makefile.in and ../scripts/run_make_pbsim.sh"  
PBSIM_HOME='../src'
RESDIR_HOME='../results/'
USER=`whoami`

make_pbsim_cl() {
  echo "MAKE PBSim + CL tracking"
  curdir=`pwd`
  cd $PBSIM_HOME
  echo $AUTO > Makefile.out
  cat cl.config >> Makefile.out
  cat Makefile.in >> Makefile.out
  make clean; make -f Makefile.out
  cd $curdir
}

make_pbsim_wp() {
  echo "MAKE PBSim + WP"
  curdir=`pwd`
  cd $PBSIM_HOME
  echo $AUTO > Makefile.out
  cat wp.config >> Makefile.out
  cat Makefile.in >> Makefile.out
  make clean; make -f Makefile.out
  cd $curdir
}

make_pbsim_notracking() {
  echo "MAKE PBSim + notracking"
  curdir=`pwd`
  cd $PBSIM_HOME
  echo $AUTO > Makefile.out
  cat notracking.config >> Makefile.out
  cat Makefile.in >> Makefile.out
  make clean; make -f Makefile.out
  cd $curdir
}

make_pbsim_sleep() {
  echo "MAKE PBSim + sleep"
  curdir=`pwd`
  cd $PBSIM_HOME
  echo $AUTO > Makefile.out
  cat notracking_sleep.config >> Makefile.out
  cat Makefile.in >> Makefile.out
  make clean; make -f Makefile.out
  cd $curdir
}


make_pbsim_report_times() {
  echo "MAKE PBSim + report_times"
  curdir=`pwd`
  cd $PBSIM_HOME
  echo $AUTO > Makefile.out
  cat report_times.config >> Makefile.out
  cat Makefile.in >> Makefile.out
  make clean; make -f Makefile.out
  cd $curdir
}

init() {
  sudo modprobe msr
}

init

#!/bin/bash

source ./run_make_pbsim.sh
RESDIR=$RESDIR_HOME/res_$(date +%Y-%m-%d_%H-%M-%S)

make -p $RESDIR

./run_redis.sh $RESDIR
./run_metis.sh $RESDIR
./run_turi.sh $RESDIR

./create_graphs $RESDIR

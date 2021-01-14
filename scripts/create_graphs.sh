#!/bin/bash

DATADIR=../results/$1
OUTDIR=../results/$1/plots
PYSCR=read_bin.py

METISDIR_1='metis'
REDISDIR='redis'
TURIDIR='turi'

set -x 

mkdir -p $OUTDIR

# parse output data and create graphs 

# Redis amplification
python3 $PYSCR $DATADIR/$REDISDIR/redis_rand_dcl_cl.bin $OUTDIR/redis_rand.dat
python3 $PYSCR $DATADIR/$REDISDIR/redis_seq_dcl_cl.bin $OUTDIR/redis_seq.dat
python3 redis_amplif.py $OUTDIR

# wp results 
python3 read_res_pbsim.py $OUTDIR
python3 results_wp.py $OUTDIR


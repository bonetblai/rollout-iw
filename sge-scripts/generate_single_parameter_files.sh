#!/bin/bash

algorithm=$1
frameskip=$2
time_budget=$3
features=$4
subtables=$5
other=$6
roms_path=$7
roms=$8

export IW_ROOT=$HOME/software/github/rollout-iw

# each experiment is run 5 times, averages are then reported
cat $IW_ROOT/atari-roms/$roms | \
  awk '{ for( i = 0; i < 5; ++i ) printf "%s %s %s %s %s %s %s/%s\n", algorithm, frameskip, time_budget, features, ns, other, roms_path, $1;
       }' \
       algorithm=$algorithm \
       frameskip=$frameskip \
       time_budget=$time_budget \
       features=$features \
       ns=$subtables \
       other=$other \
       roms_path=$roms_path


#!/bin/bash

algorithm=$1
frameskip=$2
budget=$3
features=$4
other=$5
roms_path=$6
roms=$7

export IW_ROOT=$HOME/software/github/rollout-iw

cat $IW_ROOT/atari-roms/$roms | \
  awk '{ printf "%s %s %s %s %s %s/%s\n", algorithm, frameskip, budget, features, other, roms_path, $1;
       }' \
       algorithm=$algorithm \
       frameskip=$frameskip \
       budget=$budget \
       features=$features \
       other=$other \
       roms_path=$roms_path


#!/bin/bash

frameskip=$1
budget=$2
features=$3
other=$4
roms_path=$5
roms=$6

export IW_ROOT=$HOME/software/github/rollout-iw

cat $IW_ROOT/atari-roms/$roms | \
  awk '{ printf "%s %s %s %s %s/%s\n", frameskip, budget, features, other, roms_path, $1;
       }' \
       frameskip=$frameskip \
       budget=$budget \
       features=$features \
       other=$other \
       roms_path=$roms_path


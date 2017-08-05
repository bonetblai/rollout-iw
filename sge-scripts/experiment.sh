#!/bin/bash

frameskip=$1
budget=$2
features=$3
raw_options=$4
rom=$5
log_file=$6

export IW_ROOT=$HOME/software/github/rollout-iw
export ALE_PATH=$HOME/software/github/Arcade-Learning-Environment

ulimit -c 0

options=${raw_options//,/ }

if [ "$budget" == "inf" ]; then
  $IW_ROOT/src/rolloutIW --frameskip $frameskip --features $features --log-file $log_file --rom $IW_ROOT/atari-roms/$rom $options
else
  $IW_ROOT/src/rolloutIW --frameskip $frameskip --online-budget $budget --features $features --log-file $log_file --rom $IW_ROOT/atari-roms/$rom $options
fi


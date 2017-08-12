#!/bin/bash

algorithm=$1
frameskip=$2
budget=$3
features=$4
novelty_subtables=$5
raw_options=$6
rom=$7
log_file=$8

export IW_ROOT=$HOME/software/github/rollout-iw
export ALE_PATH=$HOME/software/github/Arcade-Learning-Environment

ulimit -c 0

options=${raw_options//,/ }

if [ "$novelty_subtables" == "0" ]; then
  $IW_ROOT/src/rom_planner --planner $algorithm --frameskip $frameskip --online-budget $budget --features $features --log-file $log_file --rom $IW_ROOT/atari-roms/$rom $options
else
  $IW_ROOT/src/rom_planner --novelty-subtables --planner $algorithm --frameskip $frameskip --online-budget $budget --features $features --log-file $log_file --rom $IW_ROOT/atari-roms/$rom $options
fi


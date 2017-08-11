#!/bin/bash

algorithm=$1
frameskip=$2
budget=$3
features=$4
raw_options=$5
rom=$6
log_file=$7

export IW_ROOT=$HOME/software/github/rollout-iw
export ALE_PATH=$HOME/software/github/Arcade-Learning-Environment

ulimit -c 0

options=${raw_options//,/ }

$IW_ROOT/src/rolloutIW --planner $algorithm --frameskip $frameskip --online-budget $budget --features $features --log-file $log_file --rom $IW_ROOT/atari-roms/$rom $options


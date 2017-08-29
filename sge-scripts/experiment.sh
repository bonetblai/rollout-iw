#!/bin/bash

algorithm=$1
frameskip=$2
time_budget=$3
features=$4
novelty_subtables=$5
raw_options=$6
rom=$7
log_file=$8

export IW_ROOT=$HOME/software/github/rollout-iw
export ALE_PATH=$HOME/software/github/Arcade-Learning-Environment

ulimit -c 0

options=${raw_options//,/ }

$IW_ROOT/src/rom_planner \
  --disable-caching \
  --random-actions \
  --alpha 1 \
  --execute-single-action \
  --break-ties-using-rewards \
  --planner $algorithm \
  --frameskip $frameskip \
  --time-budget $time_budget \
  --features $features \
  --log-file $log_file \
  --rom $IW_ROOT/atari-roms/$rom $options

#$IW_ROOT/src/rom_planner \
#  --random-actions \
#  --alpha 1 \
#  --execute-single-action \
#  --break-ties-using-rewards \
#  --planner $algorithm \
#  --frameskip $frameskip \
#  --time-budget $time_budget \
#  --features $features \
#  --log-file $log_file \
#  --rom $IW_ROOT/atari-roms/$rom $options

#$IW_ROOT/src/rom_planner \
#  --disable-caching \
#  --alpha 1000 \
#  --use-alpha-to-update-reward-for-death \
#  --execute-single-action \
#  --break-ties-using-rewards \
#  --planner $algorithm \
#  --frameskip $frameskip \
#  --time-budget $time_budget \
#  --features $features \
#  --log-file $log_file \
#  --rom $IW_ROOT/atari-roms/$rom $options

#$IW_ROOT/src/rom_planner \
#  --alpha 1000 \
#  --use-alpha-to-update-reward-for-death \
#  --execute-single-action \
#  --break-ties-using-rewards \
#  --planner $algorithm \
#  --frameskip $frameskip \
#  --time-budget $time_budget \
#  --features $features \
#  --log-file $log_file \
#  --rom $IW_ROOT/atari-roms/$rom $options



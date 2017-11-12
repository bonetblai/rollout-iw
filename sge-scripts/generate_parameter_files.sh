#!/bin/bash

parameters_path=$1
roms_path=$2
suffix=$3

algorithm=$4
frameskip=$5
budget=$6
features=$7
discounts=$8

mkdir -p $parameters_path

for alg in $algorithm; do #rollout bfs; do
  for fs in $frameskip; do #05 10 15; do
    for b in $budget; do #0.25 0.50 1.0 2.0 16.0 32.0; do
      for f in $features; do #3; do
        for d in $discounts; do #0.95 1.00
          ./generate_single_parameter_files.sh $alg $fs $b $f 0 $d "--execute-single-action,--nodisplay" $roms_path $roms_path/README.uniq > $parameters_path/alg=$alg.fs=$fs.budget=$b.features=$f.ns=0.discount=$d.$suffix.txt;
          ./generate_single_parameter_files.sh $alg $fs $b $f 1 $d "--execute-single-action,--nodisplay,--novelty-subtables" $roms_path $roms_path/README.uniq > $parameters_path/alg=$alg.fs=$fs.budget=$b.features=$f.ns=1.discount=$d.$suffix.txt;
        done;
      done;
    done;
  done;
done


#!/bin/bash

roms_path=$1
suffix=$2

mkdir -p parameters

#for alg in rollout bfs; do
#  for fs in 05 15 30; do
#    for b in inf; do
#      for f in 0 1 2 3; do
#        ./generate_single_parameter_files.sh $alg $fs $b $f "--nodisplay" $roms_path $roms_path/README.uniq > parameters/alg=$alg.fs=$fs.budget=$b.features=$f.$suffix.txt;
#      done;
#    done;
#  done;
#done

for alg in rollout bfs; do
  for fs in 05 10 15 30; do
    for b in inf 1.0 2.0 4.0 8.0 16.0; do
      for f in 0 1 2 3; do
        ./generate_single_parameter_files.sh $alg $fs $b $f 0 "--execute-single-action,--nodisplay" $roms_path $roms_path/README.uniq > parameters/alg=$alg.fs=$fs.budget=$b.features=$f.ns=0.$suffix.txt;
        ./generate_single_parameter_files.sh $alg $fs $b $f 1 "--execute-single-action,--nodisplay,--novelty-subtables" $roms_path $roms_path/README.uniq > parameters/alg=$alg.fs=$fs.budget=$b.features=$f.ns=1.$suffix.txt;
      done;
    done;
  done;
done


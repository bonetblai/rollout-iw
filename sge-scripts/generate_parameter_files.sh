#!/bin/bash

roms=$1
suffix=$2

for fs in 05 10 20 30; do
  for b in inf; do
    for f in 0 1 2 3; do
      ./generate_single_parameter_files.sh $fs $b $f "--nodisplay" $roms > parameters/fs=$fs.budget=$b.features=$f.$suffix.txt;
    done;
  done;
done

for fs in 05 10 20 30; do
  for b in 0.5 1.0 1.5 3.0; do
    for f in 0 1 2 3; do
      ./generate_single_parameter_files.sh $fs $b $f "--execute-single-action,--nodisplay" $roms > parameters/fs=$fs.budget=$b.features=$f.$suffix.txt;
    done;
  done;
done


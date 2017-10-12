#!/bin/bash

target=parameters_aaai18

./generate_parameter_files.sh $target openai-gym/group_1 openai-g1
./generate_parameter_files.sh $target openai-gym/group_2 openai-g2
./generate_parameter_files.sh $target openai-gym/group_3 openai-g3
./generate_parameter_files.sh $target ALE-Atari-Width/group_1 ale-g1
./generate_parameter_files.sh $target ALE-Atari-Width/group_2 ale-g2
./generate_parameter_files.sh $target ALE-Atari-Width/group_3 ale-g3

# remove empty parameter files
find $target -empty -exec rm {} \;


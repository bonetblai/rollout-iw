#!/bin/bash

target=$1
algorithm=`echo $2 | sed "s/ /_/g"`
frameskip=`echo $3 | sed "s/ /_/g"`
budget=`echo $4 | sed "s/ /_/g"`
features=`echo $5 | sed "s/ /_/g"`
discounts=`echo $6 | sed "s/ /_/g"`

generate() {
    my_target=$1
    my_algorithm=`echo $2 | sed "s/_/ /g"`
    my_frameskip=`echo $3 | sed "s/_/ /g"`
    my_budget=`echo $4 | sed "s/_/ /g"`
    my_features=`echo $5 | sed "s/_/ /g"`
    my_discounts=`echo $6 | sed "s/_/ /g"`

    ./generate_parameter_files.sh $my_target openai-gym/group_1 openai-g1 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
    ./generate_parameter_files.sh $my_target openai-gym/group_2 openai-g2 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
    ./generate_parameter_files.sh $my_target openai-gym/group_3 openai-g3 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
    ./generate_parameter_files.sh $my_target ALE-Atari-Width/group_1 ale-g1 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
    ./generate_parameter_files.sh $my_target ALE-Atari-Width/group_2 ale-g2 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
    ./generate_parameter_files.sh $my_target ALE-Atari-Width/group_3 ale-g3 "$my_algorithm" "$my_frameskip" "$my_budget" "$my_features" "$my_discounts"
}

generate $target $algorithm $frameskip $budget $features $discounts

# remove empty parameter files
find $target -empty -exec rm {} \;

# Used as follows for post-submission experiments
#./generate_parameters_aaai18.sh parameters_aaai18.post_submission "rollout" "05 10 15" "0.25 0.50 1.0 2.0 16.0" "3" "0.95 1.00"
#./generate_parameters_aaai18.sh parameters_aaai18.post_submission_2 "rollout" "10 15" "32.0" "3" "0.95 1.00"
#./generate_parameters_aaai18.sh parameters_aaai18.post_submission_3 "bfs" "10 15" "1.0 32.0" "3" "0.95 1.00"
#./generate_parameters_aaai18.sh parameters_aaai18.post_submission_extra "bfs" "10 15" "1.0 32.0" "1" "0.95 1.00"
#./generate_parameters_aaai18.sh parameters_aaai18.post_submission_extra "rollout" "10 15" "0.25 0.50 1.0 2.0 16.0 32.0" "1" "0.95 1.00"
#rm parameters_aaai18.post_submission_extra/*.ns=0.*


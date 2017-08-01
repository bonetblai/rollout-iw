#!/bin/bash

frameskip=$1
budget=$2
features=$3
other=$4
roms=$5

cat $roms | \
  awk '{ printf "%s %s %s %s %s\n", frameskip, budget, features, other, $1;
       }' \
       frameskip=$frameskip \
       budget=$budget \
       features=$features \
       other=$other


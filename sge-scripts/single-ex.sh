#/bin/bash

template_file=$1
parameters_file=$2
output_dir=$3
nr=`wc -l $parameters_file | awk '{ print $1; }'`

mkdir -p output
rm -f output/out.$output_dir output/err.$output_dir
sed "s/XXX/$nr/" < $template_file | sed "s/OUT/out.$output_dir/" | sed "s/ERR/err.$output_dir/" > tmp.sub
qsub tmp.sub $parameters_file $output_dir
rm tmp.sub


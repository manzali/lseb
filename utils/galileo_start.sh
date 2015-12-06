#!/bin/bash

dir=${HOME}/__ID__
hostnames=${dir}/hostnames.txt

i=0
while read line; do
  cmd="numactl --physcpubind=0-3 --localalloc ${HOME}/lseb/build/lseb -c ${dir}/configuration.json -i $i"
  ssh -n -f $line $cmd > ${dir}/lseb${i}.log 2>&1 &
  i=$(($i+1))
done < $hostnames


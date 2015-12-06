#!/bin/bash

dir=${HOME}/__ID__
hostnames=${dir}/hostnames.txt

# Kill processes
while read line; do
  cmd="killall -q lseb;"
  ssh -n $line $cmd
done < $hostnames

qdel __ID__


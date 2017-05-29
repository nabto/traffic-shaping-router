#!/bin/bash


printenv

sleep 1

mkdir -p /data

tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 &

echo $(($(date +%s%N)/1000000))

ping -i 0.05 -W 1 -c 30 google.com

sleep 5






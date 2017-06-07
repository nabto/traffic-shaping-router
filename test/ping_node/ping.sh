#!/bin/bash


printenv

sleep 1

mkdir -p /data

tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 -U &

echo $(($(date +%s%N)/1000000))

ping -i 0.05 -W 1 -c 20 google.com

dig google.com @208.67.222.222 -p 5353

wget -T 1 -t 10 www.google.com

sleep 5






#!/bin/bash


printenv

pipework --wait -i eth0

sleep 1

mkdir -p /data

tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 &

ping -W 1 -c 1 google.com

ping -i 0.05 -W 1 -c 30 ${DSTADDR}

ping -i 0.05 -W 1 -c 30 ${DSTADDR}

sleep 10






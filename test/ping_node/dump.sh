#!/bin/bash

echo "running dump.sh"

if=`ip route | grep 10.0.1.0 | awk '{print $3}'`
echo ip route | grep ${GATEWAY} | awk '{print $3}'

if [ "${if}" == "" ]; then
   echo "if was empty: ${if}"
else
    echo "if was ${if}"
    pipework --wait -i $if
fi
#sleep 1
       
mkdir -p /data

echo tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump
tcpdump -i any -s 1500 -w /data/${DUMPNAME}.dump &

sleep 1

#ping -W 1 -c 1 127.0.0.1
#ping -W 1 ${GATEWAY}

tail -f /dev/null

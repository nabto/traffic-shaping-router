#!/bin/bash

if [ "x${ALT_IP}" != "x" ]; then
    ip addr add ${ALT_IP}/24 dev eth0
fi

stunserver --mode full --primaryinterface $PRIMARY_IP --altinterface $ALT_IP
tail -f /dev/null

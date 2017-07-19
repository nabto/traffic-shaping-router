#!/bin/bash

if [ "x$GATEWAY" != "x" ]; then
    route del default
    route add default gw ${GATEWAY}
fi

sysctl -w net.ipv6.conf.all.accept_dad=0

exec "$@"

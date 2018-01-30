#!/bin/bash

if [ "x$GATEWAY" != "x" ]; then
    route del default
    route add default gw ${GATEWAY}
fi

if [ "x$NAT64" != "x" ]; then
    sysctl -w net.ipv6.conf.all.disable_ipv6=0
else
    echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
    sysctl -w net.ipv6.conf.all.disable_ipv6=0
    echo "options timeout:1" >> /etc/resolv.conf
fi

if [ "x$GATEWAY6" != "x" ]; then
    route -6 del default
    route -6 add default gw ${GATEWAY6}
    GATE=`ip -6 route | grep default | awk '{print $3}'`
    ip -6 route del default via ${GATE}
    ip -6 route add default via ${GATEWAY6}
    echo `ip -6 route`
fi

sysctl -w net.ipv6.conf.all.accept_dad=0

exec "$@"

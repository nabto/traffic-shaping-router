#!/bin/bash

echo "waiting for eth0 and eth1 to come up"
pipework --wait -i eth0
pipework --wait -i eth1
echo "eth0 and eth1 came up"

IF6=`ip -6 route | grep 2001:db8:1234::/48 | awk '{print $3}'`

if [ "x$IF6" != "x" ]; then
    ip -6 route del 2001:db8:1234::/48 dev $IF6
    ip -6 route add 2001:db8:1234::/64 dev $IF6
fi

tcpdump -i any -s 65535 -w /data/nat64.dump -W 1 -C 100000000 &

tayga --mktun
ip link set nat64 up
ip addr add 192.168.255.1 dev nat64
ip addr add 2001:db8:1234::1 dev nat64
ip route add 192.168.255.0/24 dev nat64
ip route add 2001:db8:1234:ffff::/96 dev nat64

echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward
ifout=`route | grep 10.42.0.0 | awk '{print $8}'`
/sbin/iptables -t nat -A POSTROUTING -o ${ifout} -j MASQUERADE
/sbin/iptables -A FORWARD -j ACCEPT

tayga

/etc/init.d/bind9 start

tail -f /dev/null

#!/bin/bash


printenv

echo $@

echo "Waiting for interfaces to come up:"
sleep 1
# outside is the interface which routes internet traffic.
os=`ip route show to match 8.8.8.8 | awk '{print $5}'`
is=`ip route | grep ${1} | awk '{print $3}'`


echo 1 > /proc/sys/net/ipv4/ip_forward

if [ "$PERSISTENT" != "" ]; then
	persistent_flag="--persistent"
fi

iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -A INPUT -m state --state NEW  ! -i $os -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A FORWARD ! -i $os -o $os -j ACCEPT
iptables -t nat -A POSTROUTING -o $os -j MASQUERADE $persistent_flag


trap "kill 0" SIGTERM

tail -f /dev/null


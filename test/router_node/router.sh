#!/bin/bash

printenv

echo $@

echo "Waiting for interfaces to come up:"

# outside is the interface which routes internet traffic.
os=`ip route show to match 8.8.8.8 | awk '{print $5}'`
echo ip route show to match 8.8.8.8 | awk '{print $5}'

echo "os is: ${os}"

mkdir -p /data

#tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 &

iptables -F
#iptables -t nat -F

iptables -P INPUT DROP
iptables -P FORWARD ACCEPT
iptables -A INPUT ! -i $os -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

iptables -t nat -A POSTROUTING -o $os -j MASQUERADE

echo 1 > /proc/sys/net/ipv4/ip_forward

iptables -A FORWARD -j NFQUEUE --queue-num 0
#iptables -A INPUT -p udp -j NFQUEUE --queue-num 0

echo "starting Router from bash"
router &
echo "Router started from bash"
echo $(($(date +%s%N)/1000000))


trap "kill 0" SIGTERM

tail -f /dev/null


#!/bin/bash

printenv

echo $@

echo "Waiting for interfaces to come up:"

# outside is the interface which routes internet traffic.
os=`ip route show to match 8.8.8.8 | awk '{print $5}'`
echo ip route show to match 8.8.8.8 | awk '{print $5}'
is=`ip route | grep ${PINGERIP} | awk '{print $3}'`
echo ip route | grep ${PINGERIP} | awk '{print $3}'
is2=`ip route | grep ${DSTIP} | awk '{print $3}'`
echo ip route | grep ${DSTIP} | awk '{print $3}'
echo "os is: ${os}, is is: ${is}, is2 is: ${is2}"

pipework --wait -i $os
echo "Interface $os came up"
pipework --wait -i $is
echo "Interface $is came up"
pipework --wait -i $is2
echo "Interface $is2 came up, all interfaces ok"

mkdir -p /data

tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 &

ping -W 1 -c 1 www.google.com

echo 1 > /proc/sys/net/ipv4/ip_forward

if [ "$PERSISTENT" != "" ]; then
	persistent_flag="--persistent"
fi


# iptables -P INPUT DROP
# iptables -P FORWARD DROP
# iptables -A INPUT -m state --state NEW  ! -i $os -j ACCEPT
# iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
# iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
# iptables -A FORWARD -i $is -o $os -j ACCEPT
# iptables -t nat -A POSTROUTING -o $os -j MASQUERADE $persistent_flag

#iptables -P INPUT DROP
#iptables -P FORWARD DROP
#iptables -A FORWARD -i $os -o $is -j ACCEPT
#iptables -A FORWARD -i $is -o $os -j ACCEPT
#iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
#iptables -A INPUT -p icmp -j ACCEPT
iptables -t nat -A POSTROUTING -o $os -j MASQUERADE $persistent_flag

# iptables -t nat -A PREROUTING -p tcp -i $os --dport 80 -j DNAT --to-destination 10.42.2.2:80
# iptables -A FORWARD -p tcp -d 10.42.2.2 --dport 80 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
#iptables -t nat -A PREROUTING -p tcp -i $os --dport 5201 -j DNAT --to-destination 10.42.2.2:5201
#iptables -A FORWARD -p tcp -d 10.42.2.2 --dport 5201 -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT

iptables -A FORWARD -p icmp -j NFQUEUE --queue-num 0
#iptables -A INPUT -p udp -j NFQUEUE --queue-num 0

echo "starting Router from bash"
router &
echo "Router started from bash"



trap "kill 0" SIGTERM

tail -f /dev/null


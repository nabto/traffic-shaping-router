#!/bin/bash


function usage {
    echo "usage $0 insideIp"
    exit 1
}

if [ -z $1 ]; then
  usage;
fi;

if [ -z $2 ]; then
  usage;
fi;

printenv

echo $@

# outside is the interface which routes internet traffic.
os=`ip route show to match 8.8.8.8 | awk '{print $5}'`
echo `ip route show to match 8.8.8.8 | awk '{print $5}'`
oip=`ip -4 route get 8.8.8.8 | awk {'print $7'} | tr -d '\n'`
echo `ip -4 route get 8.8.8.8 | awk {'print $7'} | tr -d '\n'`
iip=${1}

iptables -F
#iptables -t nat -F

iptables -P INPUT DROP
iptables -P FORWARD ACCEPT
iptables -A INPUT ! -i $os -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

#iptables -t nat -A POSTROUTING -o $os -p icmp -j MASQUERADE
iptables -t raw -A OUTPUT ! -s $1 -j NOTRACK

echo 1 > /proc/sys/net/ipv4/ip_forward

iptables -A FORWARD -j NFQUEUE --queue-num 0
iptables -A INPUT -j NFQUEUE --queue-num 0

echo "starting Router from bash"
echo router -d 5 -l 0 --nat-ext-ip $oip --nat-int-ip $iip &
router -d 5 -l 0 --nat-ext-ip $oip --nat-int-ip $iip &
echo $(($(date +%s%N)/1000000))


trap "kill 0" SIGTERM

tail -f /dev/null


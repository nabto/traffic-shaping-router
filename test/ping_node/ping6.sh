#!/bin/bash

ip6tables -F
ip6tables -P INPUT ACCEPT
ip6tables -P FORWARD ACCEPT
ip6tables -A OUTPUT -j NFQUEUE --queue-num 0
ip6tables -A INPUT -j NFQUEUE --queue-num 0

echo "starting Router from bash"
echo router --ext-if eth0 --ipv6 --filters StaticDelay Loss -d 5 -l 0 --accept-mode &
router --ext-if eth0 --ipv6 --filters StaticDelay Loss -d 5 -l 0 --accept-mode &

#trap "kill 0" SIGTERM

ping6 -i 0.05 -W 1 -c 20 nabto.com

tail -f /dev/null


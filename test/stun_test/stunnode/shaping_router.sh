#!/bin/bash

function usage {
    echo "usage $0 insideIp subnetInside"
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
iip=${2}
mkdir -p /data

tcpdump -i any -s 65535 -w /data/${DUMPNAME}.dump -W 1 -C 100000000 -U &

iptables -F
#iptables -t nat -F

iptables -P INPUT DROP
iptables -P FORWARD ACCEPT
iptables -A INPUT ! -i $os -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

iptables -t raw -A OUTPUT ! -s $1 -j NOTRACK

echo 1 > /proc/sys/net/ipv4/ip_forward

iptables -A FORWARD -j NFQUEUE --queue-num 0
iptables -A INPUT -j NFQUEUE --queue-num 0


echo "starting Router from bash"

FILTER="--filters Nat "
LOSS=""
DELAY=""
TBF=""
BURST=""
NATTYPECMD=""

if [[ ${NATTYPE} != "" ]]; then
    NATTYPECMD="--nat-type ${NATTYPE}"
fi

if [[ ${NETLOSS} != "" ]]; then
    FILTER="${FILTER}Loss "
    los=`echo "scale=3 ; ${NETLOSS::-1} / 100" | bc`
    LOSS=" -l ${los}"
fi

if [[ ${NETDELAY} != "" ]]; then
    FILTER="${FILTER}StaticDelay "
    DELAY=" -d ${NETDELAY}"
fi
if [[ ${NETTBFINRATE} != "" ]] || [[ ${NETTBFOUTRATE} != "" ]]; then
    FILTER="${FILTER}TokenBucketFilter "
fi
if [[ ${NETTBFINRATE} != "" ]]; then
    TBF=" --tbf-in-rate ${NETTBFINRATE}"
    if [[ ${NETTBFINMAXTOKENS} != "" ]]; then
        TBF="${TBF} --tbf-in-max-tokens ${NETTBFINMAXTOKENS}"
    fi
    if [[ ${NETTBFINMAXPACKETS} != "" ]]; then
        TBF="${TBF} --tbf-in-max-packets ${NETTBFINMAXPACKETS}"
    fi
    if [[ ${NETTBFINREDSTART} != "" ]]; then
        TBF="${TBF} --tbf-in-red-start ${NETTBFINREDSTART}"
    fi
fi
if [[ ${NETTBFOUTRATE} != "" ]]; then
    TBF="${TBF} --tbf-out-rate ${NETTBFOUTRATE}"
    if [[ ${NETTBFOUTMAXTOKENS} != "" ]]; then
        TBF="${TBF} --tbf-out-max-tokens ${NETTBFOUTMAXTOKENS}"
    fi
    if [[ ${NETTBFOUTMAXPACKETS} != "" ]]; then
        TBF="${TBF} --tbf-out-max-packets ${NETTBFOUTMAXPACKETS}"
    fi
    if [[ ${NETTBFOUTREDSTART} != "" ]]; then
        TBF="${TBF} --tbf-out-red-start ${NETTBFOUTREDSTART}"
    fi
fi

if [[ ${NETBURSTDURATION} != "" ]]; then
    FILTER="${FILTER}Burst "
    BURST=" --burst-dur ${NETBURSTDURATION}"
    if [[ ${NETBURSTSLEEP} != "" ]]; then
        BURST="${BURST} --burst-sleep ${NETBURSTSLEEP}"
    fi
fi

# removing ms from delay and % from loss to be compatible with old style
echo router --nat-ext-ip $oip --nat-int-ip $iip -e 5203 ${FILTER} ${LOSS} ${DELAY} ${TBF} ${BURST} ${NATTYPECMD}
router --nat-ext-ip $oip --nat-int-ip $iip -e 5203 ${FILTER} ${LOSS} ${DELAY} ${TBF} ${BURST} ${NATTYPECMD} &
echo $(($(date +%s%N)/1000000))


trap "kill 0" SIGTERM

tail -f /dev/null


FROM debian:stretch

RUN apt-get update && apt-get install -y openssh-server net-tools wget iproute iptables libnet-dev libnetfilter-queue-dev libboost-program-options-dev libboost-thread-dev libboost-system-dev tcpdump less libstdc++6 binutils libboost1.62-all-dev vim dnsutils gdb ethtool valgrind strace iperf3 procps psmisc kmod --force-yes

ADD pipework /usr/bin/
RUN echo 'root:nabto123' |chpasswd

COPY entrypoint.sh /entrypoint.sh
ADD router /usr/bin/
COPY dump.sh /usr/bin/dump.sh
COPY ping.sh /usr/bin/ping.sh
COPY ping6.sh /usr/bin/ping6.sh

RUN chmod +x /entrypoint.sh
RUN chmod +x /usr/bin/router
RUN chmod +x /usr/bin/*.sh

ENTRYPOINT ["/entrypoint.sh"]


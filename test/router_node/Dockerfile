FROM tianon/debian:testing
RUN rm -rf /var/lib/apt/lists && mkdir /var/lib/apt/lists
RUN apt-get update && apt-get install -y openssh-server net-tools wget iproute iptables libnet-dev libnetfilter-queue-dev libboost-program-options-dev libboost-thread-dev libboost-system-dev tcpdump less libstdc++6 binutils libboost1.62-all-dev vim dnsutils gdb ethtool valgrind strace --force-yes

RUN mkdir -p /var/run/sshd
RUN mkdir -p /var/log/supervisor
RUN mkdir -p /root

RUN echo 'root:nabto123' |chpasswd
ADD pipework /bin/
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ADD router.sh /usr/bin/
RUN chmod +x /usr/bin/*.sh
ADD router /usr/bin/
RUN chmod +x /usr/bin/router

ENTRYPOINT ["/entrypoint.sh"]


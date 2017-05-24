FROM tianon/debian:testing
RUN apt-get update && apt-get install -y openssh-server net-tools wget iproute iptables libnet-dev libnetfilter-queue-dev libboost-program-options-dev libboost-thread-dev libboost-system-dev tcpdump less libstdc++6 binutils libboost1.62-all-dev --force-yes

RUN mkdir -p /var/run/sshd
RUN mkdir -p /var/log/supervisor
RUN mkdir -p /root

RUN echo 'root:nabto123' |chpasswd

ADD router.sh /usr/bin/
RUN chmod +x /usr/bin/*.sh
ADD pipework /bin/
ADD router /usr/bin/
RUN chmod +x /usr/bin/router


COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]


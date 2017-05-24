FROM tianon/debian:jessie

RUN apt-get update && apt-get install -y iproute net-tools iptables libgoogle-perftools-dev dnsutils psmisc procps  tcpdump less --force-yes

ADD pipework /usr/bin/
COPY ping.sh /usr/bin/ping.sh
COPY dump.sh /usr/bin/dump.sh
RUN chmod +x /usr/bin/*.sh

RUN echo 'root:nabto123' |chpasswd

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]


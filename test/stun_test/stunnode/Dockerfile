FROM debian:testing

RUN apt-get update && apt-get install stun-server stun-client net-tools iptables libnet-dev libnetfilter-queue-dev libboost-program-options-dev libboost-thread-dev libboost-system-dev libstdc++6 binutils libboost1.62-all-dev stuntman-server psmisc procps tcpdump -y

ADD entrypoint.sh stun.sh stunclient.sh router.sh inet_router.sh router /usr/bin/

RUN chmod +x /usr/bin/*.sh
RUN chmod +x /usr/bin/router

ENTRYPOINT ["/usr/bin/entrypoint.sh"]
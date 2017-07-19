FROM debian:testing

RUN apt-get update && apt-get install stun-server stun-client net-tools -y

ADD entrypoint.sh stun.sh /usr/bin/

RUN chmod +x /usr/bin/*.sh

ENTRYPOINT "/usr/bin/entrypoint.sh"
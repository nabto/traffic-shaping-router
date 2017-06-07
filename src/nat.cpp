#include "nat.hpp"
#include "packet.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


Nat::Nat(std::string ifOut){
    ifOut_ = ifOut;
    std::cout << "ifOut was: " << ifOut_ << std::endl;
    if(ifOut_ == "eth1"){
        dnatIf_ = "eth0";
    } else {
        dnatIf_ = "eth1";
    }
    dnatIp_ = 167772674;
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, ifOut_.c_str(), IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* display result */
    ipOut_ = inet_network(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    std::cout << "IP of " << ifOut_ << " is " << inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr) << " or in uint32: " << ipOut_ << std::endl;
}
Nat::~Nat(){}
void Nat::handlePacket(Packet pkt) {
    if(pkt.getProtocol() == PROTO_ICMP){
        next_->handlePacket(pkt);
        return;
    }
    if(pkt.getSourceIP() == dnatIp_) { 
        std::cout << "setting srcIp to " << ipOut_ << std::endl;
        pkt.setSourceIP(ipOut_);
    } else {
        std::cout << "setting dstIp to " << dnatIp_ << std::endl;
        pkt.setDestinationIP(dnatIp_);
        pkt.setOutboundInterface("eth1");

    }
    next_->handlePacket(pkt);
    std::cout << "nat dumping pkt" << std::endl;
    pkt.dump();
}


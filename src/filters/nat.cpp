#include "nat.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <netdb.h>
#include <tuple>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>



std::ostream& operator<<(std::ostream& os, const ConnectionTuple& ct) {
    os << "srcIp: " << ct.srcIp_ << " dstIp: " << ct.dstIp_ << " sport: " << ct.sport_ << " dport: " << ct.dport_;
    return os;
}

Nat::Nat(){
    type_ = PORT_R_NAT;
    connectionTimeout_ = 10*60; // default 10 min
}


Nat::~Nat(){}

void Nat::handlePacket(std::shared_ptr<ParentPacket> parPkt) {
    PacketPtr pkt = std::dynamic_pointer_cast<Packet>(parPkt);
#ifdef TRACE_LOG
    std::cout << "new packet: " << std::endl;
    pkt->dump();
#endif

    if(pkt->isIngoing()){
        if(filter_->filterPacket(pkt)){
            if(mapper_->mapPacket(pkt)){
                next_->handlePacket(pkt);
                return;
            }
        }
    } else{
        if(mapper_->mapPacket(pkt)){
            if(filter_->filterPacket(pkt)){
                next_->handlePacket(pkt);
                return;
            }
        }
    }
}

void Nat::setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort){
    ConnectionTuple tuple(0,inet_network(ip.c_str()),0,intPort,0);
    dnatRules_[extPort] = tuple;
}

void Nat::removeDnatRule(uint16_t extPort){
    for (auto it = dnatRules_.begin(); it != dnatRules_.end();){
        if(it->first == extPort){
            it = dnatRules_.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

void Nat::setIPs(std::string ipExt, std::string ipInt) {
    ipExt_ = inet_network(ipExt.c_str());
    ipInt_ = inet_network(ipInt.c_str());
}
void Nat::setTimeout(uint32_t to){
    connectionTimeout_ = to;
}

void Nat::setNatType(std::string type) {
    if (type.compare("portr") == 0){
        type_ = PORT_R_NAT;
    } else if(type.compare("addrr") == 0){
        type_= ADDR_R_NAT;
    } else if(type.compare("symnat") == 0){
        type_= SYM_NAT;
    } else if(type.compare("fullcone") == 0){
        type_= FULL_CONE_NAT;
    } else {
        std::cout << "Nat type was invalid not changing it. Nat type was: " << type << std::endl;
    }
}

bool Nat::init() {
    if (type_ == PORT_R_NAT) {
        filter_ = std::make_shared<PortrSymNatFilter>(connectionTimeout_);
        mapper_ = std::make_shared<NatMapper>(ipInt_, ipExt_, connectionTimeout_);
        std::cout << "Nat filter initialized as Port Restricted Nat" << std::endl;
    } else if(type_ == ADDR_R_NAT) {
        filter_ = std::make_shared<AddrrNatFilter>(connectionTimeout_);
        mapper_ = std::make_shared<NatMapper>(ipInt_, ipExt_, connectionTimeout_);
        std::cout << "Nat filter initialized as Address Restricted Nat" << std::endl;
    } else if(type_ == SYM_NAT) {
        filter_ = std::make_shared<PortrSymNatFilter>(connectionTimeout_);
        mapper_ = std::make_shared<SymNatMapper>(ipInt_, ipExt_, connectionTimeout_);
        std::cout << "Nat filter initialized as Symetric Nat" << std::endl;
    } else if(type_ == FULL_CONE_NAT) {
        filter_ = std::make_shared<FullconeNatFilter>(connectionTimeout_);
        mapper_ = std::make_shared<NatMapper>(ipInt_, ipExt_, connectionTimeout_);
        std::cout << "Nat filter initialized as Fullcone Nat" << std::endl;
    } else {
        std::cout << "Nat init failed: type was invalid. Nat type was: " << type_ << std::endl;
        return false;
    }
    mapper_->setDnatRules(dnatRules_);
    filter_->setDnatRules(dnatRules_);
    return true;
}


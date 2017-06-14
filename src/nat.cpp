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


#define CONNECTION_TIMEOUT 10 // seconds for now

std::ostream& operator<<(std::ostream& os, const ConnectionTuple& ct) {
    os << "srcIp: " << ct.srcIp_ << " dstIp: " << ct.dstIp_ << " sport: " << ct.sport_ << " dport: " << ct.dport_;
    return os;
}

Nat::Nat(){
}


Nat::~Nat(){}

void Nat::removeFromMaps(ConnectionTuple tuple){
    std::lock_guard<std::mutex> lock(mutex_);
    for(size_t i = 0; i < intConn_.size(); i ++){
        if (intConn_[i].first == tuple){
            intConn_.erase(intConn_.begin()+i);
            extConn_.erase(extConn_.begin()+i);
            break;
        }
    }
}

void Nat::handlePacket(PacketPtr pkt) {
    auto tuple = ConnectionTuple(pkt);
    if(pkt->getProtocol() == PROTO_ICMP){
        // Using ICMP id like port numbers for nat purposes
        tuple.setSport(pkt->getIcmpId());
        tuple.setDport(pkt->getIcmpId());
    }
    if(pkt->getSourceIP() == ipInt_) {
        // INTERNAL PACKET
        std::lock_guard<std::mutex> lock(mutex_);
        int i = findIntConn(tuple);
        if(i != -1){
            // Connection found performing SNAT
            auto ent = intConn_[i].second.lock();
            ent->rearm();
            pkt->setSourceIP(extConn_[i].first.getDstIP());
            pkt->setSourcePort(extConn_[i].first.getDport());
            next_->handlePacket(pkt);
#ifdef TRACE_LOG
            std::cout << "found Connection for internal packet passing it with:\n\tsrcIp: "
                      << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
            return;
        }
        // ============================
        // IF PORT NUMBER SHOULD CHANGE
        // THROUGH THE ROUTER IT SHOULD
        // BE DONE HERE!!
        // ============================
        auto extTuple = ConnectionTuple(pkt->getDestinationIP(),
                                        ipExt_,
                                        tuple.getDport(),
                                        tuple.getSport(),
                                        pkt->getProtocol());
        makeNewConn(extTuple,tuple);
        pkt->setSourceIP(extTuple.getDstIP());
        pkt->setSourcePort(extTuple.getDport());
        
#ifdef TRACE_LOG
        std::cout << "Made new connection for internal packet. Passing packet with:\n\tsrcIp: "
                  << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
        next_->handlePacket(pkt);
    } else {
        // EXTERNAL PACKET
        std::lock_guard<std::mutex> lock(mutex_);
        int i = findExtConn(tuple);
        if(i != -1) {
            // Connection found
            auto ent = extConn_[i].second.lock();
            ent->rearm();
            pkt->setDestinationIP(intConn_[i].first.getSrcIP());
            pkt->setDestinationPort(intConn_[i].first.getSport());
            next_->handlePacket(pkt);
#ifdef TRACE_LOG
            std::cout << "found Connection for external packet passing it with:\n\tdstIp: "
                      << pkt->getDestinationIP() << "\n\tdport: " << pkt->getDestinationPort()
                      << std::endl;
#endif
            return;
        }else if(dnatRules.count(pkt->getDestinationPort()) == 1){
            auto intTuple = ConnectionTuple(dnatRules[pkt->getDestinationPort()].getDstIP(),
                                            tuple.getSrcIP(),
                                            dnatRules[pkt->getDestinationPort()].getDport(),
                                            tuple.getSport(),
                                            tuple.getProto());
            makeNewConn(tuple, intTuple);
            pkt->setDestinationIP(intTuple.getSrcIP());
            pkt->setDestinationPort(intTuple.getSport());
            next_->handlePacket(pkt);

#ifdef TRACE_LOG
            std::cout << "Found DNAT rule passing packet with:\n\tdstIp: " << pkt->getDestinationIP()
                      << "\n\tdport: " << pkt->getDestinationPort() << std::endl;
#endif
            
        }
#ifdef TRACE_LOG
        std::cout << "Dropping external packet, no connection found for tuple: " << std::endl
                  << tuple << std::endl;
#endif
    }
}

void Nat::setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort){
    ConnectionTuple tuple(0,inet_network(ip.c_str()),0,intPort,0);
    dnatRules[extPort] = tuple;
}

void Nat::removeDnatRule(uint16_t extPort){
    for (auto it = dnatRules.begin(); it != dnatRules.end();){
        if(it->first == extPort){
            it = dnatRules.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

void Nat::setIPs(std::string ipExt, std::string ipInt) {
    ipExt_ = inet_network(ipExt.c_str());
    ipInt_=inet_network(ipInt.c_str());
}

void Nat::makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup){
    auto self = shared_from_this();

    ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(CONNECTION_TIMEOUT),
                                                  [self,intTup](ConnectionEntry* obj){
                                                      self->removeFromMaps(intTup);
                                                      delete obj;
                                                  });
    intConn_.push_back(std::make_pair(intTup,entry));
    extConn_.push_back(std::make_pair(extTup,entry));
    entry->start();
}


int Nat::findIntConn(ConnectionTuple tup){
    for(size_t i = 0 ; i < intConn_.size(); i++){
        if(intConn_[i].first == tup){
            // Connection found
            return i;
        }
    }
    return -1;
}

int Nat::findExtConn(ConnectionTuple tup){
    for(size_t i = 0 ; i < extConn_.size(); i++){
        if(extConn_[i].first == tup){
            // Connection found
            return i;
        }
    }
    return -1;
}


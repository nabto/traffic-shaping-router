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
//#define TRACE_LOG

Nat::Nat(std::string ipExt, std::string ipInt): ipExt_(inet_network(ipExt.c_str())), ipInt_(inet_network(ipInt.c_str())){
    std::cout << std::endl << " ======== NAT INIT ======= " << std::endl;
    std::cout << " ipExt: " << ipExt_ << " intIp: " << ipInt_ << std::endl;
}


Nat::~Nat(){}

void Nat::removeFromMaps(ConnectionTuple tuple){
    std::lock_guard<std::mutex> lock(mutex_);
    for(size_t i = 0; i < intConn_.size(); i ++){
        if (intConn_[i].first == tuple){
            intConn_.erase(intConn_.begin()+i);
            extConn_.erase(extConn_.begin()+i);
#ifdef TRACE_LOG
            std::cout << "intMap size from remove: " << intConn_.size() << std::endl;
            std::cout << "extMap size from remove: " << extConn_.size() << std::endl;
#endif
            break;
        }
    }
}

void Nat::handlePacket(PacketPtr pkt) {
#ifdef TRACE_LOG
    std::cout << "nat dumping pkt" << std::endl;
    pkt->dump();
#endif
    auto tuple = ConnectionTuple(pkt);
    if(pkt->getProtocol() == PROTO_ICMP){
        // Using ICMP id like port numbers for nat purposes
        tuple.setSport(pkt->getIcmpId());
        tuple.setDport(pkt->getIcmpId());
    }
    if(pkt->getSourceIP() == ipInt_) {
#ifdef TRACE_LOG
        std::cout << "Internal Packet" << std::endl;
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        for(size_t i = 0 ; i < intConn_.size(); i++){
            if(intConn_[i].first == tuple){
                // Connection found
                auto ent = intConn_[i].second.lock();
                ent->rearm();
                pkt->setSourceIP(extConn_[i].first.getDstIP());
                pkt->setSourcePort(extConn_[i].first.getDport());
                next_->handlePacket(pkt);
#ifdef TRACE_LOG
                std::cout << "found Connection passing it with:\n\tsrcIp: " << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
                return;
            }
        }
#ifdef TRACE_LOG
        std::cout << "Connection not found, making new:" << std::endl;
#endif
        auto self = shared_from_this();
        ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(CONNECTION_TIMEOUT, pkt),
                                                      [self,tuple](ConnectionEntry* obj){
                                                          self->removeFromMaps(tuple);
                                                          delete obj;
                                                      });
        entry->start();
        intConn_.push_back(std::make_pair(tuple,entry));

        // ============================
        // IF PORT NUMBER SHOULD CHANGE
        // THROUGH THE ROUTER IT SHOULD
        // BE DONE HERE!!
        // ============================
        auto extTuple = ConnectionTuple(pkt->getDestinationIP(), ipExt_, tuple.getDport(), tuple.getSport(), pkt->getProtocol());
        extConn_.push_back(std::make_pair(extTuple,entry));
        pkt->setSourceIP(ipExt_);
#ifdef TRACE_LOG
        std::cout << "Passing packet with:\n\tsrcIp: " << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
        next_->handlePacket(pkt);
    } else {
#ifdef TRACE_LOG
        std::cout << "External Packet" << std::endl;
#endif
        std::lock_guard<std::mutex> lock(mutex_);
        if(dnatRules.count(pkt->getDestinationPort()) == 1){
#ifdef TRACE_LOG
            std::cout << "Invoking Dnat rule" << std::endl;
#endif
            pkt->setDestinationIP(dnatRules[pkt->getDestinationPort()].getDstIP());
            pkt->setDestinationPort(dnatRules[pkt->getDestinationPort()].getDport());
            next_->handlePacket(pkt);
#ifdef TRACE_LOG
            std::cout << "passing packet with:\n\tdstIp: " << pkt->getDestinationIP() << "\n\tdport: " << pkt->getDestinationPort() << std::endl;
#endif
            
        } else {
#ifdef TRACE_LOG
            std::cout << "Looking for connection match for tuple:" << std::endl;
            std::cout << "\t" << tuple << std::endl;
#endif
            for(size_t i = 0 ; i < extConn_.size(); i++){
#ifdef TRACE_LOG
                std::cout << "checking tuple: \n\t" << extConn_[i].first << std::endl;
#endif
                if(extConn_[i].first == tuple){
                    // Connection found
                    auto ent = extConn_[i].second.lock();
                    ent->rearm();
                    pkt->setDestinationIP(intConn_[i].first.getSrcIP());
                    pkt->setDestinationPort(intConn_[i].first.getSport());
                    next_->handlePacket(pkt);
#ifdef TRACE_LOG
                    std::cout << "found Connection passing it with:\n\tdstIp: " << pkt->getDestinationIP() << "\n\tdport: " << pkt->getDestinationPort() << std::endl;
#endif
                    return;
                }
            }
        }
    }
}

int Nat::setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort){
    ConnectionTuple tuple(0,inet_network(ip.c_str()),0,intPort,0);
    dnatRules[extPort] = tuple;
    return 0;
}

int Nat::removeDnatRule(uint16_t extPort){
    for (auto it = dnatRules.begin(); it != dnatRules.end();){
        if(it->first == extPort){
            it = dnatRules.erase(it);
            return 0;
        } else {
            ++it;
        }
    }
    return -1;
}

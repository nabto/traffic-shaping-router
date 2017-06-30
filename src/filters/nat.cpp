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

void Nat::removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup){
    std::lock_guard<std::mutex> lock(mutex_);
    intConn_.erase(intTup);
    extConn_.erase(extTup);
}

void Nat::handlePacket(PacketPtr pkt) {
#ifdef TRACE_LOG
    std::cout << "new packet: " << std::endl;
    pkt->dump();
#endif
    auto tuple = ConnectionTuple(pkt);
    if(pkt->getProtocol() == PROTO_ICMP){
        // Using ICMP id like port numbers for nat purposes
        tuple.setSport(pkt->getIcmpId());
        tuple.setDport(pkt->getIcmpId());
    }
    if(pkt->getSourceIP() == ipInt_) {
        // INTERNAL PACKET
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(intConn_.find(tuple) != intConn_.end()){
                // Connection found performing SNAT
                ent = intConn_[tuple].lock();
            }
        }
        if(ent) {
            ent->rearm();
            pkt->setSourceIP(ent->getExtTup().getDstIP());
            pkt->setSourcePort(ent->getExtTup().getDport());
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
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(extConn_.find(tuple) != extConn_.end()){
                ent = extConn_[tuple].lock();
            }
        }
        if(ent) {
            // Connection found
            ent->rearm();
            pkt->setDestinationIP(ent->getIntTup().getSrcIP());
            pkt->setDestinationPort(ent->getIntTup().getSport());
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
    ipInt_ = inet_network(ipInt.c_str());
}

void Nat::makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup){
    auto self = shared_from_this();

    ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(CONNECTION_TIMEOUT, extTup, intTup),
                                                  [self](ConnectionEntry* obj){
                                                      self->removeFromMaps(obj->getIntTup(), obj->getExtTup());
                                                      delete obj;
                                                  });
    intConn_[intTup] = entry;
    extConn_[extTup] = entry;
    entry->start();
}

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

//#define TRACE_LOG

std::ostream& operator<<(std::ostream& os, const ConnectionTuple& ct) {
    os << "srcIp: " << ct.srcIp_ << " dstIp: " << ct.dstIp_ << " sport: " << ct.sport_ << " dport: " << ct.dport_;
    return os;
}

Nat::Nat(){
    type_ = PORT_R_NAT;
}


Nat::~Nat(){}

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
            if(comIntConn_.find(tuple) != comIntConn_.end()){
                // Connection found performing SNAT
                ent = comIntConn_[tuple].lock();
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
        // Connection not found, making a new one
        uint16_t extDport = tuple.getSport();
        if(type_ == SYM_NAT){
#ifdef TRACE_LOG
            std::cout << "Nat type was SYM_NAT, checking port " << extDport <<  " is not taken." << std::endl;
            dumpConnMaps();
#endif
            std::lock_guard<std::mutex> lock(mutex_);
            SymNatIntTuple redTup(tuple);
            if(symNatIntConn_.find(redTup) != symNatIntConn_.end()){
                // Port number already used by host
                while(symNatIntConn_.find(redTup) != symNatIntConn_.end()) {
                    extDport = redTup.getSport()+1;
                    redTup.setSport(extDport);
                }
            }
#ifdef TRACE_LOG
            else {
                std::cout << "port " << extDport << " was not taken" << std::endl;
            }
#endif
        }
        auto extTuple = ConnectionTuple(pkt->getDestinationIP(),
                                        ipExt_,
                                        tuple.getDport(),
                                        extDport,
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
        if(type_ == SYM_NAT || type_ == PORT_R_NAT) {
            std::lock_guard<std::mutex> lock(mutex_);
            if(comExtConn_.find(tuple) != comExtConn_.end()){
                // connection found
                ent = comExtConn_[tuple].lock();
            }
        } else if (type_ == ADDR_R_NAT) {
            std::lock_guard<std::mutex> lock(mutex_);
            AddrrExtTuple redExtTup(tuple);
            if(addrrExtConn_.find(redExtTup) != addrrExtConn_.end()){
                ent = addrrExtConn_[redExtTup].lock();
            }
        } else if (type_ == FULL_CONE_NAT) {
            std::lock_guard<std::mutex> lock(mutex_);
            FullconeExtTuple redExtTup(tuple);
            if(fcExtConn_.find(redExtTup) != fcExtConn_.end()){
                ent = fcExtConn_[redExtTup].lock();
            }
        } else {
#ifdef TRACE_LOG
            std::cout << "Nat type was invalid dropping packet. Nat type was: " << type_ << std::endl;
#endif
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
            ConnectionTuple intTuple(dnatRules[pkt->getDestinationPort()].getDstIP(),
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
#ifdef TRACE_LOG
            std::cout << "Nat type was invalid not changing it. Nat type was: " << type_ << std::endl;
#endif
    }
    comIntConn_.clear();
    comExtConn_.clear();
    symNatIntConn_.clear();
    addrrExtConn_.clear();
    fcExtConn_.clear();
}

void Nat::makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup){
    auto self = shared_from_this();

    ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(CONNECTION_TIMEOUT, extTup, intTup),
                                                  [self](ConnectionEntry* obj){
                                                      self->removeFromMaps(obj->getIntTup(), obj->getExtTup());
                                                      delete obj;
                                                  });
    comIntConn_[intTup] = entry;
    comExtConn_[extTup] = entry;
    if(type_ == SYM_NAT){
        SymNatIntTuple redTup(intTup);
        symNatIntConn_[redTup] = entry;
    } else if(type_ == PORT_R_NAT) {
    } else if(type_ == ADDR_R_NAT) {
        AddrrExtTuple redTup(extTup);
        addrrExtConn_[redTup] = entry;
    } else if(type_ == FULL_CONE_NAT) {
        FullconeExtTuple redTup(extTup);
        fcExtConn_[redTup] = entry;
    } else {
#ifdef TRACE_LOG
        std::cout << "Nat type was invalid in makeNewConn. Nat type was: " << type_ << std::endl;
#endif
    }
    entry->start();
}

void Nat::removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup){
    std::lock_guard<std::mutex> lock(mutex_);
    comIntConn_.erase(intTup);
    comExtConn_.erase(extTup);
    if(type_ == SYM_NAT){
        SymNatIntTuple redTup(intTup);
        symNatIntConn_.erase(redTup);
    } else if(type_ == PORT_R_NAT) {
    } else if(type_ == ADDR_R_NAT) {
        AddrrExtTuple redTup(extTup);
        addrrExtConn_.erase(redTup);
    } else if(type_ == FULL_CONE_NAT) {
        FullconeExtTuple redTup(extTup);
        fcExtConn_.erase(redTup);
    } else {
#ifdef TRACE_LOG
        std::cout << "Nat type was invalid in removeFromMaps. Nat type was: " << type_ << std::endl;
#endif
    }
}

void Nat::dumpConnMaps(){
    std::cout << "dumpint complete internal connections" << std::endl;
    for( auto it : comIntConn_) {
        std::cout << "\t" << it.first << std::endl;
    }
    std::cout << "dumpint complete external connections" << std::endl;
    for( auto it : comExtConn_) {
        std::cout << "\t" << it.first << std::endl;
    }
    std::cout << "dumpint sym nat internal connections" << std::endl;
    for( auto it : symNatIntConn_) {
        std::cout << "\t" << it.first << std::endl;
    }
    std::cout << "dumpint addrr external connections" << std::endl;
    for( auto it : addrrExtConn_) {
        std::cout << "\t" << it.first << std::endl;
    }
    std::cout << "dumpint fullcone external connections" << std::endl;
    for( auto it : fcExtConn_) {
        std::cout << "\t" << it.first << std::endl;
    }
}

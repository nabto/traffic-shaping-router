
#include "nat_mapper.hpp"

//#define TRACE_LOG

///================ NAT MAPPER BASE CLASS IMPL ================ ///
void NatMapperBase::setDnatRules(std::map<uint16_t, ConnectionTuple> rules){
    dnatRules_ = rules;
}

void NatMapperBase::removeDnatRule(uint16_t extPort){
    for (auto it = dnatRules_.begin(); it != dnatRules_.end();){
        if(it->first == extPort){
            it = dnatRules_.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

bool NatMapperBase::checkDnat(PacketPtr pkt){
    if(pkt->isIngoing()){
        if(dnatRules_.count(pkt->getDestinationPort()) == 1){
            pkt->setDestinationIP(dnatRules_[pkt->getDestinationPort()].getDstIP());
            pkt->setDestinationPort(dnatRules_[pkt->getDestinationPort()].getDport());
            

#ifdef TRACE_LOG
            std::cout << "Found DNAT rule for ingoing packet passing with:\n\tdstIp: " << pkt->getDestinationIP()
                      << "\n\tdport: " << pkt->getDestinationPort() << std::endl;
#endif
            return true;
        }
    } else {
        if(dnatRules_.count(pkt->getSourcePort()) == 1){
            pkt->setSourceIP(ipExt_);
#ifdef TRACE_LOG
            std::cout << "Found DNAT rule for outgoing packet passing with:\n\tsrcIp: " << pkt->getSourceIP()
                      << "\n\tsport: " << pkt->getSourcePort() << std::endl;
#endif
            return true;
        }
    }        
    return false;
}



///================ NAT MAPPER CLASS IMPL ======================= ///
NatMapper::NatMapper(uint32_t ipInt, uint32_t ipExt, uint32_t ct) {
    ipInt_ = ipInt;
    ipExt_ = ipExt;
    connectionTimeout_ = ct;
}

NatMapper::~NatMapper() {

}

bool NatMapper::mapPacket(PacketPtr pkt) {
    auto tuple = ConnectionTuple(pkt);
    if(checkDnat(pkt)){
        return true;
    }
    if(pkt->isIngoing()){
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(extPortMap_.find(pkt->getDestinationPort()) != extPortMap_.end()){
                // Connection found performing SNAT
                ent = extPortMap_[pkt->getDestinationPort()].lock();
            }
        }
        if(ent) {
            // Connection found
            pkt->setDestinationIP(ent->getIntTup().getSrcIP());
            pkt->setDestinationPort(ent->getIntTup().getSport());
            return true;
        }
#ifdef TRACE_LOG
        std::cout << "Dropping external packet, no connection found for tuple: " << std::endl
                  << tuple << std::endl;
#endif
        return false;
    } else {
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(intPortMap_.find(pkt->getSourcePort()) != intPortMap_.end()){
                // Connection found performing SNAT
                ent = intPortMap_[pkt->getSourcePort()].lock();
            }
        }
        if(ent) {
            ent->rearm();
            pkt->setSourceIP(ent->getExtTup().getDstIP());
            pkt->setSourcePort(ent->getExtTup().getDport());
            return true;
        }
        // Connection not found, making a new one
        auto extTuple = ConnectionTuple(pkt->getDestinationIP(),
                                        ipExt_,
                                        pkt->getDestinationPort(),
                                        tuple.getSport(),
                                        pkt->getProtocol());
        makeNewConn(extTuple,tuple);
        pkt->setSourceIP(extTuple.getDstIP());
        pkt->setSourcePort(extTuple.getDport());
        
#ifdef TRACE_LOG
        std::cout << "Made new connection for internal packet. Passing packet with:\n\tsrcIp: "
                  << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
        return true;
    }
        
}

void NatMapper::makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup){
    auto self = shared_from_this();

    ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(connectionTimeout_, extTup, intTup),
                                                  [self](ConnectionEntry* obj){
                                                      self->removeFromMaps(obj->getIntTup(), obj->getExtTup());
                                                      delete obj;
                                                  });
    intPortMap_[intTup.getSport()] = entry;
    extPortMap_[extTup.getDport()] = entry;
    entry->start();
}

void NatMapper::removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup){
    std::lock_guard<std::mutex> lock(mutex_);
    intPortMap_.erase(intTup.getSport());
    extPortMap_.erase(extTup.getDport());
}

///================ SYM NAT MAPPER CLASS IMPL ======================= ///
SymNatMapper::SymNatMapper(uint32_t ipInt, uint32_t ipExt, uint32_t ct) {
    ipInt_ = ipInt;
    ipExt_ = ipExt;
    connectionTimeout_ = ct;
}

SymNatMapper::~SymNatMapper() {

}

bool SymNatMapper::mapPacket(PacketPtr pkt) {
    auto tuple = ConnectionTuple(pkt);
    if(checkDnat(pkt)){
        return true;
    }
    if(pkt->isIngoing()){
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(extPortMap_.find(pkt->getDestinationPort()) != extPortMap_.end()){
                // Connection found performing SNAT
                ent = extPortMap_[pkt->getDestinationPort()].lock();
            }
        }
        if(ent) {
            // Connection found
            pkt->setDestinationIP(ent->getIntTup().getSrcIP());
            pkt->setDestinationPort(ent->getIntTup().getSport());
            return true;
        }
#ifdef TRACE_LOG
        std::cout << "Symnat Dropping external packet, no connection found for tuple: " << std::endl
                  << tuple << std::endl;
#endif
        return false;
    } else {
        std::shared_ptr<ConnectionEntry> ent;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(intPortMap_.find(tuple) != intPortMap_.end()){
                // Connection found performing SNAT
                ent = intPortMap_[tuple].lock();
            }
        }
        if(ent) {
            ent->rearm();
            pkt->setSourceIP(ent->getExtTup().getDstIP());
            pkt->setSourcePort(ent->getExtTup().getDport());
            return true;
        }
        // Connection not found, making a new one
        auto extTuple = ConnectionTuple(pkt->getDestinationIP(),
                                        ipExt_,
                                        pkt->getDestinationPort(),
                                        mapPort(tuple),
                                        pkt->getProtocol());
        makeNewConn(extTuple,tuple);
        pkt->setSourceIP(extTuple.getDstIP());
        pkt->setSourcePort(extTuple.getDport());
        
#ifdef TRACE_LOG
        std::cout << "Symnat Made new connection for internal packet. Passing packet with:\n\tsrcIp: "
                  << pkt->getSourceIP() << "\n\tSport: " << pkt->getSourcePort() << std::endl;
#endif
        return true;
    }
        
}

uint16_t SymNatMapper::mapPort(ConnectionTuple tup) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint16_t port = tup.getSport();
    while(extPortMap_.find(port) != extPortMap_.end()){
        port++;
    }
    return port;
}

void SymNatMapper::makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup) {
    auto self = shared_from_this();

    ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(connectionTimeout_, extTup, intTup),
                                                  [self](ConnectionEntry* obj){
                                                      self->removeFromMaps(obj->getIntTup(), obj->getExtTup());
                                                      delete obj;
                                                  });
    intPortMap_[intTup] = entry;
    extPortMap_[extTup.getDport()] = entry;
    entry->start();
}

void SymNatMapper::removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup){
    std::lock_guard<std::mutex> lock(mutex_);
    intPortMap_.erase(intTup);
    extPortMap_.erase(extTup.getDport());
}


#include "nat_filters.hpp"


void dumpConnMap(std::map<ConnectionTuple, ConnectionEntryWeakPtr> map){
    std::cout << "dumping map:" << std::endl;
    for( auto it : map) {
        std::cout << "\t" << it.first << std::endl;
    }
}


// =============== PORT RESTRICTED / SYM NAT IMPL =============== //

bool PortrSymNatFilter::filterPacket(PacketPtr pkt){
    auto tuple = ConnectionTuple(pkt);
    if(pkt->isIngoing()){
#ifdef TRACE_LOG
        std::cout << "Portr/Sym filtering ingoing packet" << std::endl;
#endif
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(connMap_.find(tuple) != connMap_.end()){
                return true;
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(dnatRules_.find(pkt->getDestinationPort()) != dnatRules_.end()){
                return true;
            }
        }
    } else {
#ifdef TRACE_LOG
        std::cout << "Portr/Sym filtering outgoing packet" << std::endl;
#endif
        std::shared_ptr<ConnectionEntry> ent;
        ConnectionTuple extTup(pkt->getDestinationIP(),
                               pkt->getSourceIP(),
                               pkt->getDestinationPort(),
                               pkt->getSourcePort(),
                               pkt->getProtocol());
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(connMap_.find(extTup) != connMap_.end()){
                ent = connMap_[extTup].lock();;
            }
        }
        if(ent){
            ent->rearm();
            return true;
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            auto self = shared_from_this();
            ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(connectionTimeout_, extTup, tuple),
                                                          [self](ConnectionEntry* obj){
                                                              self->removeFromMaps(obj->getExtTup());
                                                              delete obj;
                                                          });
            connMap_[extTup] = entry;
            entry->start();
            return true;
        }
        
    }
    return false;
}

void PortrSymNatFilter::removeFromMaps(ConnectionTuple tup){
    std::lock_guard<std::mutex> lock(mutex_);
    if(connMap_.find(tup) != connMap_.end()){
        connMap_.erase(tup);
    }
}  


// =============== ADDRESS RESTRICTED FILTER IMPL =============== //

bool AddrrNatFilter::filterPacket(PacketPtr pkt){
    auto tuple = AddrrTuple(pkt);
    if(pkt->isIngoing()){
#ifdef TRACE_LOG
        std::cout << "Addrr filtering ingoing packet" << std::endl;
#endif
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(connMap_.find(tuple) != connMap_.end()){
                return true;
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(dnatRules_.find(pkt->getDestinationPort()) != dnatRules_.end()){
                return true;
            }
        }
    } else {
#ifdef TRACE_LOG
        std::cout << "Addrr filtering outgoing packet" << std::endl;
#endif
        std::shared_ptr<ConnectionEntry> ent;
        AddrrTuple extTup(pkt->getDestinationIP(),
                               pkt->getSourceIP(),
                               pkt->getDestinationPort(),
                               pkt->getSourcePort(),
                               pkt->getProtocol());
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(connMap_.find(extTup) != connMap_.end()){
                ent = connMap_[extTup].lock();;
            }
        }
        if(ent){
            ent->rearm();
            return true;
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            auto self = shared_from_this();
            ConnectionEntryPtr entry = ConnectionEntryPtr(new ConnectionEntry(connectionTimeout_, extTup, tuple),
                                                          [self](ConnectionEntry* obj){
                                                              self->removeFromMaps(obj->getExtTup());
                                                              delete obj;
                                                          });
            connMap_[extTup] = entry;
            entry->start();
            return true;
        }
        
    }
    return false;
}
void AddrrNatFilter::removeFromMaps(AddrrTuple tup){
    std::lock_guard<std::mutex> lock(mutex_);
    if(connMap_.find(tup) != connMap_.end()){
        connMap_.erase(tup);
    }
}  



// ==================== FULLCONE NAT IMPL ==================== //

bool FullconeNatFilter::filterPacket(PacketPtr pkt){
    return true;
}



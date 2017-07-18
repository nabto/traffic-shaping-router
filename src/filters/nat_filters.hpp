#ifndef NAT_FILTERS_HPP
#define NAT_FILTERS_HPP

#include <packet.hpp>
#include "connection_entry.hpp"
#include "connection_tuple.hpp"
#include <mutex>

class NatFilter
{
 public:
    NatFilter() {}
    ~NatFilter() {}
    virtual bool filterPacket(PacketPtr pkt) = 0;
    void setDnatRules(std::map<uint16_t, ConnectionTuple> rules){dnatRules_ = rules;}
    void removeDnatRule(uint16_t extPort){dnatRules_.erase(extPort);}
 protected:
    uint32_t connectionTimeout_;
    std::mutex mutex_;
    std::map<uint16_t, ConnectionTuple> dnatRules_;
};

class PortrSymNatFilter : public NatFilter, public std::enable_shared_from_this<PortrSymNatFilter>
{
 public:
    PortrSymNatFilter(uint32_t to){connectionTimeout_ = to;}
    ~PortrSymNatFilter(){}
    bool filterPacket(PacketPtr pkt);
 protected:
    std::map<SymNatTuple, ConnectionEntryWeakPtr> connMap_;
    void removeFromMaps(ConnectionTuple tup);
};

class AddrrNatFilter : public NatFilter, public std::enable_shared_from_this<AddrrNatFilter>
{
 public:
    AddrrNatFilter(uint32_t to){connectionTimeout_ = to;}
    ~AddrrNatFilter(){}
    bool filterPacket(PacketPtr pkt);
 protected:
    std::map<AddrrTuple, ConnectionEntryWeakPtr> connMap_;
    void removeFromMaps(ConnectionTuple tup);
};

class FullconeNatFilter : public NatFilter, public std::enable_shared_from_this<FullconeNatFilter>
{
 public:
    FullconeNatFilter(uint32_t to){connectionTimeout_ = to;}
    ~FullconeNatFilter(){}
    bool filterPacket(PacketPtr pkt);
};

#endif // NAT_FILTERS_HPP

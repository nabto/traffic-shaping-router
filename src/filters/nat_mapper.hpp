#ifndef NAT_MAPPER_HPP
#define NAT_MAPPER_HPP

#include <packet.hpp>
#include "connection_entry.hpp"
#include "connection_tuple.hpp"
#include <mutex>

class NatMapperBase
{
 public:
    virtual bool mapPacket(PacketPtr pkt) = 0;
    void setDnatRules(std::map<uint16_t, ConnectionTuple> rules);
    void removeDnatRule(uint16_t extPort);
 protected:
    uint32_t ipInt_;
    uint32_t ipExt_;
    std::mutex mutex_;
    uint32_t connectionTimeout_;
    std::map<uint16_t, ConnectionTuple> dnatRules_;

    bool checkDnat(PacketPtr pkt);
};

class NatMapper : public NatMapperBase, public std::enable_shared_from_this<NatMapper>
{
 public:
    NatMapper(uint32_t intIp, uint32_t extIp, uint32_t ct);
    ~NatMapper();
    virtual bool mapPacket(PacketPtr pkt);
 protected:
    std::map<uint16_t, ConnectionEntryWeakPtr> intPortMap_;
    std::map<uint16_t, ConnectionEntryWeakPtr> extPortMap_;

    void makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup);
    void removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup);
};
class SymNatMapper : public NatMapperBase, public std::enable_shared_from_this<SymNatMapper>
{
 public:
    SymNatMapper(uint32_t intIp, uint32_t extIp, uint32_t ct);
    ~SymNatMapper();
    virtual bool mapPacket(PacketPtr pkt);
 protected:
    std::map<ConnectionTuple, ConnectionEntryWeakPtr> intPortMap_;
    std::map<uint16_t, ConnectionEntryWeakPtr> extPortMap_;

    void makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup);
    void removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup);
    uint16_t mapPort(ConnectionTuple tup);
};

#endif // NAT_MAPPER_HPP

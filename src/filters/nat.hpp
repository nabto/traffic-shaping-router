#ifndef NAT_HPP
#define NAT_HPP

#include <filter.hpp>
#include <packet.hpp>
#include "connection_entry.hpp"
#include "connection_tuple.hpp"
#include <mutex>

enum nat_type { PORT_R_NAT, ADDR_R_NAT, SYM_NAT, FULL_CONE_NAT};

// Nat Filter implementing port restricted nat for a single internal IP.
class Nat : public Filter, public std::enable_shared_from_this<Nat>
{
 public:
    Nat();
    ~Nat();
    // Function handling incoming packets
    void handlePacket(PacketPtr pkt);

    // Functions for Nat configuration 
    void setIPs(std::string ipExt, std::string ipInt);
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort);
    void removeDnatRule(uint16_t extPort);
    void setNatType(std::string type);
 private:
    uint32_t ipExt_;
    uint32_t ipInt_;
    std::mutex mutex_;
    std::map<ConnectionTuple, ConnectionEntryWeakPtr> comIntConn_;
    std::map<ConnectionTuple, ConnectionEntryWeakPtr> comExtConn_;
    std::map<SymNatIntTuple, ConnectionEntryWeakPtr> symNatIntConn_;
    std::map<AddrrExtTuple, ConnectionEntryWeakPtr> addrrExtConn_;
    std::map<FullconeExtTuple, ConnectionEntryWeakPtr> fcExtConn_;
    std::map<uint16_t, ConnectionTuple> dnatRules;
    nat_type type_;

    void makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup);
    void removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup);

    void dumpConnMaps();
};

#endif // NAT_HPP

#ifndef NAT_HPP
#define NAT_HPP

#include <filter.hpp>
#include <packet.hpp>
#include "connection_entry.hpp"
#include "connection_tuple.hpp"
#include <mutex>


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

 private:
    uint32_t ipExt_;
    uint32_t ipInt_;
    std::mutex mutex_;
    std::map<ConnectionTuple, ConnectionEntryWeakPtr> intConn_;
    std::map<ConnectionTuple, ConnectionEntryWeakPtr> extConn_;
    std::map<uint16_t, ConnectionTuple> dnatRules;

    void makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup);
    void removeFromMaps(ConnectionTuple intTup, ConnectionTuple extTup);
};

#endif // NAT_HPP

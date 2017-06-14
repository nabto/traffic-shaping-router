#ifndef NAT_HPP
#define NAT_HPP

#include "filter.hpp"
#include "packet.hpp"
#include "connectionEntry.hpp"
#include "connectionTuple.hpp"
#include <mutex>

class Nat : public Filter, public std::enable_shared_from_this<Nat>
{
 public:
    Nat();
    ~Nat();
    void handlePacket(PacketPtr pkt);
    void setIPs(std::string ipExt, std::string ipInt);
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort);
    void removeDnatRule(uint16_t extPort);

 private:
    uint32_t ipExt_;
    uint32_t ipInt_;
    std::mutex mutex_;
    std::vector<std::pair<ConnectionTuple,ConnectionEntryWeakPtr>> intConn_;
    std::vector<std::pair<ConnectionTuple,ConnectionEntryWeakPtr>> extConn_;
    std::map<uint16_t, ConnectionTuple> dnatRules;

    void makeNewConn(ConnectionTuple extTup, ConnectionTuple intTup);
    void removeFromMaps(ConnectionTuple tuple);
    int findIntConn(ConnectionTuple tup);
    int findExtConn(ConnectionTuple tup);
};

#endif // NAT_HPP

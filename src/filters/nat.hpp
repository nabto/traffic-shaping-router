#ifndef NAT_HPP
#define NAT_HPP

#include <filter.hpp>
#include <packet.hpp>
#include "connection_entry.hpp"
#include "connection_tuple.hpp"
#include "nat_mapper.hpp"
#include "nat_filters.hpp"
#include <mutex>

#ifndef CONNECTION_TIMEOUT
#define CONNECTION_TIMEOUT 100 // seconds for now
#endif
enum nat_type { PORT_R_NAT, ADDR_R_NAT, SYM_NAT, FULL_CONE_NAT};

// Nat Filter implementing nat for a single internal IP.
class Nat : public Filter, public std::enable_shared_from_this<Nat>
{
 public:
    Nat();
    ~Nat();
    // Function handling incoming packets
    void handlePacket(std::shared_ptr<ParentPacket> pkt);

    // Functions for Nat configuration 
    void setIPs(std::string ipExt, std::string ipInt);
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort);
    void removeDnatRule(uint16_t extPort);
    void setNatType(std::string type);
    void setTimeout(uint32_t to);

    // Init function must be called to implement the configuration parameters
    bool init();
 private:
    uint32_t ipExt_;
    uint32_t ipInt_;
    uint32_t connectionTimeout_;
    nat_type type_;
    std::map<uint16_t, ConnectionTuple> dnatRules_;
    std::shared_ptr<NatMapperBase> mapper_;
    std::shared_ptr<NatFilter> filter_;
};

#endif // NAT_HPP

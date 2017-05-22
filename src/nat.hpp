#ifndef NAT_HPP
#define NAT_HPP

#include "filter.hpp"
#include "packet.hpp"

class Nat : public Filter
{
 public:
    Nat();
    ~Nat();
    void handlePacket(Packet pkt);
};

#endif // NAT_HPP

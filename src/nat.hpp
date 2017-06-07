#ifndef NAT_HPP
#define NAT_HPP

#include "filter.hpp"
#include "packet.hpp"

class Nat : public Filter
{
 public:
    Nat(std::string ifOut);
    ~Nat();
    void handlePacket(Packet pkt);

 private:
    std::string ifOut_;
    uint32_t ipOut_;

    std::string dnatIf_;
    uint32_t dnatIp_;
};

#endif // NAT_HPP

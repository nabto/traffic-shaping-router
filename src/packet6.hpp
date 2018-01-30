#ifndef PACKET6_H // one-time include
#define PACKET6_H

#include "parent_packet.hpp"
#include <netinet/in.h>

// Packet class containing an IPv6 packet for processing
class Packet6;
typedef std::shared_ptr<Packet6> Packet6Ptr;

class Packet6 : public ParentPacket, public std::enable_shared_from_this<Packet6>
{
 public:
    Packet6(struct nfq_q_handle *qh,struct nfq_data *nfa);
    // Construct dummy packet for testing purposes
    Packet6();
    ~Packet6();

    // set functions for changing IPs, Ports, and interfaces
    void setSourceIP(struct in6_addr ip) {srcIp_ = ip;}
    void setDestinationIP(struct in6_addr ip) {dstIp_ = ip;}
    void setSourcePort(uint16_t sport){sport_ = sport;}
    void setDestinationPort(uint16_t dport){dport_ = dport;}
    void setOutboundInterface(const std::string & out) {strOutboundInterface_ = out;}

    // Get functions for packet information
    const struct in6_addr getSourceIP() const {return srcIp_;}
    const struct in6_addr getDestinationIP() const {return dstIp_;}
    const uint32_t getFlowLabel() const {return flow_ & 0x000FFFFF;}
    const uint32_t getVersion() const   {return flow_ & 0xF0000000;}
    const uint32_t getTclass() const    {return flow_ & 0x0FF00000;}
    const uint16_t getPayloadLen() const {return plen_;}
    const uint8_t getNextHeader() const {return nxt_;}
    const uint8_t getHopLimit() const {return hlim_;}

    void dump();

 private:
    uint32_t flow_;
    uint16_t plen_;
    uint8_t nxt_;
    uint8_t hlim_;
    struct in6_addr srcIp_;
    struct in6_addr dstIp_;
};

#endif

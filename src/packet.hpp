#ifndef PACKET_H // one-time include
#define PACKET_H

#include "parent_packet.hpp"


// Packet class containing a packet for processing
class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Packet : public ParentPacket, public std::enable_shared_from_this<Packet>
{
 public:
    Packet(struct nfq_q_handle *qh,struct nfq_data *nfa);
    // Construct dummy packet for testing purposes
    Packet();
    Packet(uint32_t srcIp, uint32_t dstIp, uint16_t sport, uint16_t dport);
    ~Packet();
		
    // set functions for changing IPs, Ports, and interfaces
    void setSourceIP(uint32_t ip) {srcIp_ = ip;}
    void setDestinationIP(uint32_t ip) {dstIp_ = ip;}
    void setOutboundInterface(const std::string & out) {strOutboundInterface_ = out;}

    // Get functions for packet information
    const uint32_t getSourceIP() const {return srcIp_;}
    const uint32_t getDestinationIP() const {return dstIp_;}
    const uint16_t getFragmentFlags() const {return ipFrag_ & 0xE000;}
    const uint16_t getFragmentID() const {return ipFrag_ & 0x1FFF;}
    const uint8_t getProtocol() const {return transProt_;}
    void dump();

 protected:
    uint8_t transProt_;
    uint32_t srcIp_;
    uint32_t dstIp_;

    uint16_t ipFrag_;
    uint8_t ipHdrLen_;
};

#endif

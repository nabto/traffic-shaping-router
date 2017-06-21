#ifndef PACKET_H // one-time include
#define PACKET_H

#include <stdint.h>

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
}
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <functional>


#define PROTO_ICMP 1
#define PROTO_TCP 6
#define PROTO_UDP 17


// Packet class containing a packet for processing
class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Packet
{
 public:
    Packet(struct nfq_data *nfa);
    // Construct dummy packet for testing purposes
    Packet();
    ~Packet();
		
    // set functions for changing IPs, Ports, and interfaces
    void setSourceIP(uint32_t ip) {srcIp_ = ip;}
    void setDestinationIP(uint32_t ip) {dstIp_ = ip;}
    void setSourcePort(uint16_t sport){sport_ = sport;}
    void setDestinationPort(uint16_t dport){dport_ = dport;}
    void setOutboundInterface(const std::string & out) {strOutboundInterface_ = out;}

    // Get functions for packet information
    const int getNetfilterID() const {return nfqID_;}
    const uint32_t getSourceIP() const {return srcIp_;}
    const uint32_t getDestinationIP() const {return dstIp_;}
    const uint16_t getSourcePort() const {return sport_;}
    const uint16_t getDestinationPort() const {return dport_;}
    const uint16_t getIcmpId() const {return icmpId_;}
    const uint16_t getFragmentFlags() const {return ipFrag_ & 0xE000;}
    const uint16_t getFragmentID() const {return ipFrag_ & 0x1FFF;}
    const uint8_t getProtocol() const {return transProt_;}
    const uint32_t getPacketLen() const {return (uint32_t)packetDataLen_;}
    const std::vector<uint8_t> getPacketData() const {return packetData_;}
    void getInboundInterface(std::string & in) const {in = strInboundInterface_;}
    void getOutboundInterface(std::string & out) const {out = strOutboundInterface_;}

    // Function dumping packet data to stdout for debugging
    void dump();
    // Timestamp is first set in the constructor, this can be retrieved and reset here
    void resetTimeStamp(){stamp_ = boost::posix_time::microsec_clock::local_time();}
    boost::posix_time::ptime getTimeStamp(){return stamp_;}

 protected:
    boost::posix_time::ptime stamp_;
    std::vector<uint8_t> packetData_;
    std::string	strInboundInterface_;
    std::string	strOutboundInterface_;
    int packetDataLen_;
    int nfqID_;

    uint8_t transProt_;
    uint32_t srcIp_;
    uint32_t dstIp_;
    uint16_t sport_;
    uint16_t dport_;
    uint16_t icmpId_;

    uint16_t ipFrag_;
    uint8_t ipHdrLen_;
};

#endif

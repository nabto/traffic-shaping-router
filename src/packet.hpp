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

struct ipParams
{
    uint16_t ipLen;
    uint8_t	 ipTos;
    uint16_t ipId;
    uint16_t ipFrag;
    uint8_t  ipTtl;
    uint8_t  ipProt;
    uint32_t ipSrc;
    uint32_t ipDst;
    uint8_t  ipHdrLen;    
};

struct transParams
{
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
};

struct tcpParams
{
    uint32_t seq;
    uint32_t ack;
    uint8_t control;
    uint16_t win;
    uint16_t urg;
    const uint8_t * tcpPayload;
    uint32_t tcpPayloadSize;
    uint8_t tcpOptionsSize;
    uint8_t * tcpOptions;
};

struct icmpParams
{
    uint8_t icmpType;
    uint8_t icmpCode;
    uint16_t icmpId;
};

class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Packet
{
 public:

    Packet(struct nfq_data *nfa);
    // Construct empty packet for testing purposes
    Packet();
    ~Packet();
		
    
    void setSourceIP(uint32_t ip) {srcIp_ = ip;}
    void setDestinationIP(uint32_t ip) {dstIp_ = ip;}
    void setSourcePort(uint16_t sport){sport_ = sport;}
    void setDestinationPort(uint16_t dport){dport_ = dport;}
    void setOutboundInterface(const std::string & out) {strOutboundInterface_ = out;}


    void processAsync();

    const int getNetfilterID() const {return nfqID_;}
    const uint32_t getSourceIP() const {return srcIp_;}
    const uint32_t getDestinationIP() const {return dstIp_;}
    const uint16_t getSourcePort() const {return sport_;}
    const uint16_t getDestinationPort() const {return dport_;}
    const uint16_t getIcmpId() const {return icmpId_;}
    const uint16_t getFragmentFlags() const {return ipFrag_ & 0xE000;}
    const uint16_t getFragmentID() const {return ipFrag_ & 0x1FFF;}
    const uint8_t getProtocol() const {return transProt_;}
    const std::vector<uint8_t> getPacketData() const {return packetData_;}
    void getInboundInterface(std::string & in) const {in = strInboundInterface_;}
    void getOutboundInterface(std::string & out) const {out = strOutboundInterface_;}

    
    void dump();
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

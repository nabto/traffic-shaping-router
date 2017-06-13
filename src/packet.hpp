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
		
    
    void setSourceIP(uint32_t);
    void setDestinationIP(uint32_t);
    void setSourcePort(uint16_t sport){trans_.sport = sport;}
    void setDestinationPort(uint16_t dport){trans_.dport = dport;}
    void setOutboundInterface(const std::string & out);


    void processAsync();

    const int getNetfilterID() const;
    const uint32_t getSourceIP() const;
    const uint32_t getDestinationIP() const;
    const uint16_t getSourcePort() const {return trans_.sport;}
    const uint16_t getDestinationPort() const {return trans_.dport;}
    const uint16_t getIcmpId() const {return icmp_.icmpId;}
    const uint16_t getFragmentFlags() const;
    const uint16_t getFragmentID() const;
    const uint8_t getProtocol() const;
    const struct ipParams getIpParams() const {return ip_;}
    const struct transParams getTransParams() const {return trans_;}
    const struct icmpParams getIcmpParams() const {return icmp_;}
    const struct tcpParams getTcpParams() const {return tcp_;}
    const std::vector<uint8_t> getPacketData() const {return packetData_;}
    void getInboundInterface(std::string & in) const;
    void getOutboundInterface(std::string & out) const;

    
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
    
    struct ipParams ip_;
    struct transParams trans_;
    struct icmpParams icmp_;
    struct tcpParams tcp_;
 private:
    void dumpMem(unsigned char* p,int len);
};

#endif

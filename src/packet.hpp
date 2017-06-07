#ifndef PACKET_H // one-time include
#define PACKET_H

#include <stdint.h>

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
//#include <libnetfilter_queue/libipq.h>
//#include <linux/netfilter.h>
}
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <functional>


#define PROTO_ICMP 1
#define PROTO_TCP 6
#define PROTO_UDP 17

typedef enum _ROUTER_STATUS
{
    S_OK,
    E_FAILED,
    E_UNDEFINED,
    E_INVALID_PROTOCOL
} ROUTER_STATUS;

typedef union _ipAddr
{
    uint32_t      raw;
    unsigned char octet[4];
} ipAddr;

typedef struct _rawPacket
{
    uint8_t  nVersionLength;
    uint8_t	 flagsTOS;
    uint16_t nPacketLength;
    uint16_t nFragmentID;
    uint16_t nFragFlagsOffset;
    uint8_t	 TTL;
    uint8_t	 nProtocol;
    uint16_t nHeaderChecksum;
    ipAddr   srcIP;
    ipAddr   dstIP;

    unsigned char data[1500];
} rawPacket;

class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Packet
{
 public:

    Packet(struct nfq_data *nfa);
    // Construct empty packet for testing purposes
    Packet();
    ~Packet();
		
    const int getNetfilterID() const;
    const uint32_t getSourceIP() const;
    void setSourceIP(uint32_t);
    void setDestinationIP(uint32_t);
    const uint32_t getDestinationIP() const;
    const uint16_t getFragmentFlags() const;
    const uint16_t getFragmentID() const;
		
    const uint8_t getProtocol() const;
		
    void getInboundInterface(std::string & in) const;
    void getOutboundInterface(std::string & out) const;
    void setOutboundInterface(const std::string & out);

    void processAsync();
    ROUTER_STATUS send();

    void dump();
    void resetTimeStamp(){stamp_ = boost::posix_time::microsec_clock::local_time();}
    boost::posix_time::ptime getTimeStamp(){return stamp_;}

 protected:
    boost::posix_time::ptime stamp_;
    rawPacket packetData_;
    std::string	strInboundInterface_;
    std::string	strOutboundInterface_;
    int packetDataLen_;
    struct nfqnl_msg_packet_hdr ph_;

    

    // ==== IP PARAMETERS ====
    uint16_t ipLen_;
    uint8_t	 tos_;
    uint16_t id_;
    uint16_t frag_;
    uint8_t ttl_;
    uint8_t prot_;
    uint16_t sum_;
    uint32_t src_;
    uint32_t dst_;

    //libnet_ptag_t libnet_build_tcp (uint16_t sp, uint16_t dp, uint32_t seq, uint32_t
    //ack, uint8_t control, uint16_t win, uint16_t sum, uint16_t urg, uint16_t len,
    //const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
 
    uint16_t sport_;
    uint16_t dport_;

    // ==== TCP PARAMETERS ====
    uint32_t seq_;
    uint32_t ack_;
    uint8_t control_;
    uint16_t win_;
    uint16_t tcpSum_;
    uint16_t urg_;
    uint16_t len_;
    const uint8_t * tcpPayload_;
    uint32_t tcpPayloadSize_;
    uint8_t tcpOptionsSize_;
    uint8_t * tcpOptions_;
 private:
    void dumpMem(unsigned char* p,int len);
    short getPacketHeaderLength() const;

};

#endif

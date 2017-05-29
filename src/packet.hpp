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
#include "structures.h"

#define PROTO_ICMP 1
#define PROTO_TCP 6
#define PROTO_UDP 17

typedef enum _verdict
{
    DROPPED,
    ACCEPTED,
    READY_TO_SEND,
    WAITING
} verdict;

typedef union _ipAddr
{
	uint32_t	raw;
	unsigned char	octet[4];
} ipAddr;

typedef struct _rawPacket
{
	uint8_t		nVersionLength;
	uint8_t		flagsTOS;
	uint16_t	nPacketLength;
	uint16_t	nFragmentID;
	uint16_t	nFragFlagsOffset;
	uint8_t		TTL;
	uint8_t		nProtocol;
	uint16_t	nHeaderChecksum;
	ipAddr		srcIP;
	ipAddr		dstIP;
	
	unsigned char	data[1500];
} rawPacket;

typedef struct _udpPacket
{
	uint16_t srcPort;
	uint16_t dstPort;
	uint16_t nLength;
	uint16_t nChecksum;

	unsigned char	data[];
} udpPacket;

typedef struct _tcpPacket
{
	uint16_t srcPort;
	uint16_t dstPort;
	uint32_t nSeqNum;
	uint32_t nAckId;
	uint32_t nHeaderLenFlags; // Combined header length, Reserved, and flag bits
	uint16_t nWindowSize;
	uint16_t nUrgPtr;
	
	unsigned char	data[];

} tcpPacket;

class Packet
{
 public:

    Packet(struct nfq_data *nfa);
    // Construct empty packet for testing purposes
    Packet();//{stamp_ = boost::posix_time::microsec_clock::local_time();};
    ~Packet();
		
    const int getNetfilterID() const;
    const uint32_t getSourceIP() const;
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
    struct nfq_data* m_nfData;
    rawPacket m_pPacketData;
    rawPacket* orgPktDataPtr_;
    int m_nPacketDataLen;
    std::string	m_strInboundInterface;
    std::string	m_strOutboundInterface;
    struct nfqnl_msg_packet_hdr *orgPktHead_;
    struct nfqnl_msg_packet_hdr ph_;
	
		
    void calcIPchecksum();
    void calcUDPchecksum();
    void calcTCPchecksum();

	
 private:
    void dumpMem(unsigned char* p,int len);
    short getPacketHeaderLength() const;


};

#endif

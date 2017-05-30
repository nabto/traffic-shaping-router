#include <iostream>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <libnet.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include "packet.hpp"


Packet::Packet(struct nfq_data *nfa) : stamp_(boost::posix_time::microsec_clock::local_time()) {
    m_nfData = nfa;
    u_int32_t ifi;
    char buf[IF_NAMESIZE];

    orgPktHead_ = nfq_get_msg_packet_hdr(m_nfData);
    std::memcpy(&ph_,orgPktHead_,sizeof(struct nfqnl_msg_packet_hdr));
    m_nPacketDataLen = nfq_get_payload(m_nfData, (uint8_t**)&orgPktDataPtr_);
    std::memcpy(&m_pPacketData, orgPktDataPtr_, m_nPacketDataLen);

	ifi = nfq_get_indev(m_nfData);
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			m_strInboundInterface.clear();
			m_strInboundInterface = buf;
		}
	}

	ifi = nfq_get_outdev(m_nfData);
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			m_strOutboundInterface.clear();
			m_strOutboundInterface = buf;
		}
	}
}

// Dummy packet for testing purposes
Packet::Packet() : stamp_(boost::posix_time::microsec_clock::local_time()) {
    m_pPacketData.nVersionLength = 2;
    m_pPacketData.flagsTOS = 0;
    m_pPacketData.nPacketLength = 100;
    m_pPacketData.nFragmentID = 3;
    m_pPacketData.nFragFlagsOffset = 0;
    m_pPacketData.TTL = 55;
    m_pPacketData.nProtocol = PROTO_ICMP;
    m_pPacketData.nHeaderChecksum = 0;
    m_pPacketData.srcIP.raw = 202;
    m_pPacketData.srcIP.octet[0] = 65;
    m_pPacketData.srcIP.octet[1] = 65;
    m_pPacketData.srcIP.octet[2] = 65;
    m_pPacketData.srcIP.octet[3] = 65;
    m_pPacketData.dstIP.raw = 202;
    m_pPacketData.dstIP.octet[0] = 65;
    m_pPacketData.dstIP.octet[1] = 65;
    m_pPacketData.dstIP.octet[2] = 65;
    m_pPacketData.dstIP.octet[3] = 65;
    ph_.packet_id = 5;
    m_strInboundInterface = "eth8";
    m_strOutboundInterface = "eth9";
}

Packet::~Packet() {
}

const int Packet::getNetfilterID() const {
    int id = 0;
    id = ntohl(ph_.packet_id);
    return id;
}

ROUTER_STATUS Packet::send() {
    ROUTER_STATUS ret = E_UNDEFINED;
    libnet_t *l;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ip_ptag = 0;
    int count;
    std::string strOutboundInterface;

    l = libnet_init(LIBNET_RAW4,(char*)m_strOutboundInterface.c_str(),errbuf);

    if (l == NULL) {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        ret = E_FAILED;
    } else {
        ret = S_OK;
    }

	if (ret == S_OK) {
		// Build packet
		// Advance pointer past any options headers.
		unsigned char* pData = (m_pPacketData.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H;

		ip_ptag = libnet_build_ipv4(
			LIBNET_IPV4_H + ntohs(m_pPacketData.nPacketLength) - this->getPacketHeaderLength(),                  /* length */
			m_pPacketData.flagsTOS,		/* TOS */
			this->getFragmentID(),			/* IP fragment ID */
			this->getFragmentFlags(),		/* IP Frag flags*/
			m_pPacketData.TTL,				/* TTL */
			m_pPacketData.nProtocol,		/* protocol */
			0,								/* checksum (let libnet calculate) */
			m_pPacketData.srcIP.raw,		/* source IP */
			m_pPacketData.dstIP.raw,		/* destination IP */
			pData,							/* payload */
			ntohs(m_pPacketData.nPacketLength) - this->getPacketHeaderLength(),                                  /* payload size */
			l,								/* libnet handle */
			ip_ptag);						/* libnet id */


		if (ip_ptag == -1) {
			fprintf(stderr, "Can't build IP header: %s\n", libnet_geterror(l));
			ret = E_FAILED;
	    } else {
			ret = S_OK;
		}

		// TODO: Add IP options (if any)

		if (ret == S_OK) {
			// Write to network
			count = libnet_write(l);
			if (count == -1) {
				fprintf(stderr, "Write error: %s\n", libnet_geterror(l));
				ret = E_FAILED;
			} else {
				ret = S_OK;
			}
		}

		libnet_destroy(l);
	}
	return ret;
}

void Packet::dump() {
    printf("\tid is %i\n", getNetfilterID());
    printf("\tInbound interface: %s\n",m_strInboundInterface.c_str());
    printf("\tOutbound interface: %s\n",m_strOutboundInterface.c_str());
    printf("\ttime stamp was: %s\n", boost::posix_time::to_simple_string(stamp_).c_str());

    // TODO: these two printfs will break if Network order != Host order
    printf("\tSource IP: %d.%d.%d.%d\n",
           m_pPacketData.srcIP.octet[0],
           m_pPacketData.srcIP.octet[1],
           m_pPacketData.srcIP.octet[2],
           m_pPacketData.srcIP.octet[3]);

    printf("\tDestination IP: %d.%d.%d.%d\n",
           m_pPacketData.dstIP.octet[0],
           m_pPacketData.dstIP.octet[1],
           m_pPacketData.dstIP.octet[2],
           m_pPacketData.dstIP.octet[3]);

    if (this->getProtocol() == PROTO_ICMP) {
        printf("\tICMP Packet\n");
    } else if (this->getProtocol() == PROTO_UDP) {
        unsigned char* pData = (m_pPacketData.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H;
        udpPacket* udp = (udpPacket*)pData;

        printf("\tUDP Source Port: %d\n",ntohs(udp->srcPort));
        printf("\tUDP Destination Port: %d\n",ntohs(udp->dstPort));

    } else if (this->getProtocol() == PROTO_TCP) {
        unsigned char* pData = (m_pPacketData.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H;
        tcpPacket* tcp = (tcpPacket*)pData;

        printf("\tTCP Source Port: %d\n",ntohs(tcp->srcPort));
        printf("\tTCP Destination Port: %d\n",ntohs(tcp->dstPort));

    } else {
        printf("\tUnknown Protocol\n");
    }
}


const uint8_t Packet::getProtocol() const {
    return m_pPacketData.nProtocol;
}

const uint32_t Packet::getSourceIP() const {
    return ntohl(m_pPacketData.srcIP.raw);
}

const uint32_t Packet::getDestinationIP() const {
    return ntohl(m_pPacketData.dstIP.raw);
}

void Packet::getInboundInterface(std::string & in) const {
    in = m_strInboundInterface;
}

void Packet::getOutboundInterface(std::string & out) const {
    out = m_strOutboundInterface;
}

void Packet::setOutboundInterface(const std::string & out) {
    m_strOutboundInterface = out;
}

const uint16_t Packet::getFragmentFlags() const {
    uint16_t data = ntohs(m_pPacketData.nFragFlagsOffset);
    return data & 0xE000;
}

const uint16_t Packet::getFragmentID() const {
    uint16_t data = ntohs(m_pPacketData.nFragFlagsOffset);
    return data & 0x1FFF;  // mask out first 3 bits 
}

inline short Packet::getPacketHeaderLength() const {
    return (m_pPacketData.nVersionLength & 0x0F) << 2;
}

void Packet::dumpMem(unsigned char* p,int len) {
    printf("\t\t------------------------------------------------");
    for (int i = 0 ; i < len; i++ ) {
        if (i%16==0) {
            printf("\n\t\t");
        }
        printf( "%02X:", *(p+i));
    }

    printf("\n");
    printf("\t\t------------------------------------------------\n");

}


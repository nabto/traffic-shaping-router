#include <iostream>
#include <net/if.h>
#include <stdio.h>
#include <libnet.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include "packet.hpp"

#include <netinet/ip.h>

Packet::Packet(struct nfq_q_handle *qh,struct nfq_data *nfa) {
    resetTimeStamp();
    qh_ = qh;
    struct nfq_data* nfData = nfa;
    u_int32_t ifi;
    char buf[IF_NAMESIZE];
    uint8_t* orgPktDataPtr;
    
    struct nfqnl_msg_packet_hdr *orgPktHead = nfq_get_msg_packet_hdr(nfData);
    nfqID_ = ntohl(orgPktHead->packet_id);
    packetDataLen_ = nfq_get_payload(nfData, &orgPktDataPtr);
    packetData_.clear();
    std::copy(orgPktDataPtr, (uint8_t*)orgPktDataPtr + packetDataLen_, std::back_inserter(packetData_));
    
	ifi = nfq_get_indev(nfData);
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			strInboundInterface_.clear();
			strInboundInterface_ = buf;
		}
	}

	ifi = nfq_get_outdev(nfData);
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			strOutboundInterface_.clear();
			strOutboundInterface_ = buf;
		}
	}

    iphdr* ip = (iphdr*)packetData_.data();
    ipFrag_ = ntohs(ip->frag_off);
    transProt_ = ip->protocol;
    ipHdrLen_ = ip->ihl << 2;
    srcIp_ = ntohl(ip->saddr);
    dstIp_ = ntohl(ip->daddr);

    decodeTransportProtocol(transProt_, packetData_.data()+ipHdrLen_);

#ifdef TRACE_LOG
    std::cout << "Dumping packet after construction: " << std::endl;
    dump();
#endif
}

// Dummy packet for testing purposes
Packet::Packet() {
    resetTimeStamp();
    ipFrag_ = 5;
    transProt_ = IPPROTO_TCP;
    srcIp_ = 33685514;
    dstIp_ = 33559212;
    ipHdrLen_ = 20;
    nfqID_ = 5;
    strInboundInterface_ = "eth8";
    strOutboundInterface_ = "eth9";
}

Packet::Packet(uint32_t srcIp, uint32_t dstIp, uint16_t sport, uint16_t dport) {
    resetTimeStamp();
    ipFrag_ = 5;
    transProt_ = IPPROTO_TCP;
    srcIp_ = srcIp;
    dstIp_ = dstIp;
    sport_ = sport;
    dport_ = dport;
    ipHdrLen_ = 20;
    nfqID_ = 5;
    strInboundInterface_ = "eth8";
    strOutboundInterface_ = "eth9";
}

Packet::~Packet() {
}

void Packet::dump() {
    printf("\tid is %i\n", getNetfilterID());
    printf("\tInbound interface: %s\n",strInboundInterface_.c_str());
    printf("\tOutbound interface: %s\n",strOutboundInterface_.c_str());
    printf("\ttime stamp was: %s\n", boost::posix_time::to_simple_string(stamp_).c_str());
    if(isIngoing_){
        printf("\tPacket is Ingoing\n");
    } else {
        printf("\tPacket is Outgoing\n");
    }

    // TODO: these two printfs will break if Network order != Host order
    printf("\tSource IP raw: %u\n", srcIp_);
    printf("\tSource IP: %u.%u.%u.%u\n",
           srcIp_ >> 24,
           (srcIp_ >> 16) & 0xFF,
           (srcIp_ >> 8) & 0xFF,
           srcIp_ & 0xFF);

    printf("\tDestination IP raw: %u\n", dstIp_);
    printf("\tDestination IP: %u.%u.%u.%u\n",
           dstIp_ >> 24,
           (dstIp_ >> 16) & 0xFF,
           (dstIp_ >> 8) & 0xFF,
           dstIp_ & 0xFF);

    if (transProt_ == IPPROTO_ICMP) {
        printf("\tICMP Packet\n");
        printf("\tid: %u\n", icmpId_);
    } else if (transProt_ == IPPROTO_UDP) {
        printf("\tUDP Source Port: %u\n",sport_);
        printf("\tUDP Destination Port: %u\n",dport_);
    } else if (transProt_ == IPPROTO_TCP) {
        printf("\tTCP Source Port: %d\n",sport_);
        printf("\tTCP Destination Port: %d\n",dport_);
    } else {
        printf("\tUnknown Protocol\n");
    }
}


#include <iostream>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <libnet.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include <netinet/ip6.h>

#include "packet6.hpp"


Packet6::Packet6(struct nfq_q_handle *qh,struct nfq_data *nfa) {
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
    strInboundInterface_.clear();
    strOutboundInterface_.clear();
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			strInboundInterface_ = buf;
		}
	}

	ifi = nfq_get_outdev(nfData);
	if (ifi) {
		if (if_indextoname(ifi, buf)) {
			strOutboundInterface_ = buf;
		}
	}
    ip6_hdr* ip = (ip6_hdr*)packetData_.data();
    flow_ = ip->ip6_flow;
    plen_ = ntohs(ip->ip6_plen);
    nxt_ = ip->ip6_nxt;
    hlim_ = ip->ip6_hlim;
    srcIp_ = ip->ip6_src;
    dstIp_ = ip->ip6_dst;

    decodeTransportProtocol(nxt_, packetData_.data()+40);
    
#ifdef TRACE_LOG
    std::cout << "Dumping packet after construction: " << std::endl;
    dump();
#endif
}

// Dummy packet for testing purposes
    Packet6::Packet6() {
    resetTimeStamp();
    nfqID_ = 5;
    strInboundInterface_ = "eth8";
    strOutboundInterface_ = "eth9";
}

Packet6::~Packet6() {
}

void ipv6_to_str_unexpanded(char * str, const struct in6_addr * addr);

void Packet6::dump() {
    std::cout << "\tid is " << getNetfilterID() << std::endl;
    std::cout << "\tInbound interface: " << strInboundInterface_ << std::endl;
    std::cout << "\tOutbound interface: " << strOutboundInterface_ << std::endl;
    std::cout << "\ttime stamp was: " << boost::posix_time::to_simple_string(stamp_) << std::endl;
    std::cout << "\tPayload size: " << getPayloadLen() << std::endl;
    
    if (isIngoing_) {
        std::cout << "\tPacket is Ingoing" << std::endl;
    } else {
        std::cout << "\tPacket is Outgoing" << std::endl;
    }
    // TODO: these two printfs will break if Network order != Host order
    char str[40];
    ipv6_to_str_unexpanded(str, &srcIp_);
    std::cout << "\t" << str << std::endl;
//    std::cout << "\tSource IP raw: " << srcIp_ << std::endl;

    ipv6_to_str_unexpanded(str, &dstIp_);
    std::cout << "\t" << str << std::endl;
//    std::cout << "\tDestination IP raw: " << dstIp_ << std::endl;

    if (nxt_ == IPPROTO_ICMP) {
        printf("\tICMP Packet\n");
        printf("\tid: %u\n", icmpId_);
    } else if (nxt_ == IPPROTO_ICMPV6) {
        printf("\tICMPv6 Packet\n");
        printf("\tid: %u\n", icmpId_);
    } else if (nxt_ == IPPROTO_UDP) {
        printf("\tUDP Source Port: %u\n",sport_);
        printf("\tUDP Destination Port: %u\n",dport_);
    } else if (nxt_ == IPPROTO_TCP) {
        printf("\tTCP Source Port: %d\n",sport_);
        printf("\tTCP Destination Port: %d\n",dport_);
    } else {
        std::cout << "\tUnknown Protocol: " << nxt_ << std::endl;
    }
}

void ipv6_to_str_unexpanded(char * str, const struct in6_addr * addr) {
   sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                 (int)addr->s6_addr[0], (int)addr->s6_addr[1],
                 (int)addr->s6_addr[2], (int)addr->s6_addr[3],
                 (int)addr->s6_addr[4], (int)addr->s6_addr[5],
                 (int)addr->s6_addr[6], (int)addr->s6_addr[7],
                 (int)addr->s6_addr[8], (int)addr->s6_addr[9],
                 (int)addr->s6_addr[10], (int)addr->s6_addr[11],
                 (int)addr->s6_addr[12], (int)addr->s6_addr[13],
                 (int)addr->s6_addr[14], (int)addr->s6_addr[15]);
}

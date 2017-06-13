#include <iostream>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <libnet.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include "packet.hpp"

#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <linux/tcp.h>



Packet::Packet(struct nfq_data *nfa) : stamp_(boost::posix_time::microsec_clock::local_time()) {
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
    ip_.ipLen = ntohs(ip->tot_len);
    ip_.ipTos = ip->tos;
    ip_.ipId = ntohs(ip->id);
    ip_.ipFrag = ntohs(ip->frag_off);
    ip_.ipTtl = ip->ttl;
    ip_.ipProt = ip->protocol;
    ip_.ipSrc = ntohl(ip->saddr);
    ip_.ipDst = ntohl(ip->daddr);
    ip_.ipHdrLen = ip->ihl << 2;
    
    if (ip_.ipProt == PROTO_UDP){
        udphdr* udp = (udphdr*)(packetData_.data()+ip_.ipHdrLen);
        trans_.sport = ntohs(udp->uh_sport);
        trans_.dport = ntohs(udp->uh_dport);
        trans_.len = ntohs(udp->uh_ulen);
    } else if (ip_.ipProt == PROTO_TCP){
#ifdef TRACE_LOG
        std::cout << "Dumping entire TCP packet" << std::endl;
        dumpMem((uint8_t*)&packetData_, packetDataLen_);
#endif
        tcphdr* tcp = (tcphdr *)((packetData_.data()+ip_.ipHdrLen));
        trans_.len = packetDataLen_;

        tcp_.urg = ntohs(tcp->urg_ptr);
        tcp_.win = ntohs(tcp->window);
        tcp_.control = tcp->fin | (tcp->syn << 1) | (tcp->rst << 2) | (tcp->psh << 3) | (tcp->ack << 4) | (tcp->urg << 5) | (tcp->ece << 6) | (tcp->cwr << 7);
        tcp_.ack = ntohl(tcp->ack_seq);
        tcp_.seq = ntohl(tcp->seq);
        trans_.dport = ntohs(tcp->dest);
        trans_.sport = ntohs(tcp->source);
        tcp_.tcpOptionsSize = (tcp->doff << 2) - 20;
        
        tcp_.tcpPayloadSize = packetDataLen_ - ip_.ipHdrLen - 20 - tcp_.tcpOptionsSize;
        tcp_.tcpPayload = (uint8_t*)tcp + 20 + tcp_.tcpOptionsSize;

        tcp_.tcpOptions = (uint8_t*)tcp + 20;

#ifdef TRACE_LOG
        std::cout << "TCP DATA:\n\ttcpPayloadSize_ " << tcp_.tcpPayloadSize << "\n\tlen: " << trans_.len << "\n\turg: " << tcp_.urg << "\n\twin: " << tcp_.win << "\n\tcontrol: " << unsigned(tcp_.control) << "\n\tack: " << tcp_.ack  << "\n\tseq: " << tcp_.seq << "\n\tdport: " << trans_.dport << "\n\tsport: " << trans_.sport  << "\n\tOptSize: " << unsigned(tcp_.tcpOptionsSize) << "\n\tOpt: ";
        for (int i = 0; i<tcp_.tcpOptionsSize; i++){
            std::cout << unsigned(tcp_.tcpOptions[i]) << " ";
        }
        std::cout << std::endl;
#endif
    } else if (ip_.ipProt == PROTO_ICMP){
        trans_.len = ip_.ipLen - ip_.ipHdrLen;
        icmphdr* icmp = (icmphdr*)((packetData_.data()+ip_.ipHdrLen));
        icmp_.icmpType = icmp->type;
        icmp_.icmpCode = icmp->code;
        icmp_.icmpId = icmp->un.echo.id;
    } else {
        // unknown protocol passing it raw, but NAT should drop it
        trans_.len = ip_.ipLen - ip_.ipHdrLen;
    }
}

// Dummy packet for testing purposes
Packet::Packet() : stamp_(boost::posix_time::microsec_clock::local_time()) {
    ip_.ipLen = 5;
    ip_.ipTos = 0;
    ip_.ipId = 3;
    ip_.ipFrag = 5;
    ip_.ipTtl = 55;
    ip_.ipProt = PROTO_TCP;
    ip_.ipSrc = 33685514;
    ip_.ipDst = 33559212;
    ip_.ipHdrLen = 20;
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

    // TODO: these two printfs will break if Network order != Host order
    printf("\tSource IP raw: %u\n", ip_.ipSrc);
    printf("\tSource IP: %u.%u.%u.%u\n",
           ip_.ipSrc >> 24,
           (ip_.ipSrc >> 16) & 0xFF,
           (ip_.ipSrc >> 8) & 0xFF,
           ip_.ipSrc & 0xFF);

    printf("\tDestination IP raw: %u\n", ip_.ipDst);
    printf("\tDestination IP: %u.%u.%u.%u\n",
           ip_.ipDst >> 24,
           (ip_.ipDst >> 16) & 0xFF,
           (ip_.ipDst >> 8) & 0xFF,
           ip_.ipDst & 0xFF);

    if (ip_.ipProt == PROTO_ICMP) {
        printf("\tICMP Packet\n");
        printf("\tid: %u\n", icmp_.icmpId);
    } else if (ip_.ipProt == PROTO_UDP) {
        printf("\tUDP Source Port: %u\n",trans_.sport);
        printf("\tUDP Destination Port: %u\n",trans_.dport);
        printf("\tHeader len: %u\n",trans_.len); 

    } else if (ip_.ipProt == PROTO_TCP) {
        printf("\tTCP Source Port: %d\n",trans_.sport);
        printf("\tTCP Destination Port: %d\n",trans_.dport);
        printf("\tHeader seq: %u\n",tcp_.seq); 
        printf("\tHeader ack: %u\n",tcp_.ack); 
        printf("\tHeader win: %u\n",tcp_.win); 

    } else {
        printf("\tUnknown Protocol\n");
    }
}


const int Packet::getNetfilterID() const {
    return nfqID_;
}

const uint8_t Packet::getProtocol() const {
    return ip_.ipProt;
}

const uint32_t Packet::getSourceIP() const {
    return ip_.ipSrc;
}

void Packet::setSourceIP(uint32_t ip) {
    ip_.ipSrc = ip;
}

void Packet::setDestinationIP(uint32_t ip) {
    ip_.ipDst = ip;
}

const uint32_t Packet::getDestinationIP() const {
    return ip_.ipDst;
}

void Packet::getInboundInterface(std::string & in) const {
    in = strInboundInterface_;
}

void Packet::getOutboundInterface(std::string & out) const {
    out = strOutboundInterface_;
}

void Packet::setOutboundInterface(const std::string & out) {
    strOutboundInterface_ = out;
}

const uint16_t Packet::getFragmentFlags() const {
    return ip_.ipFrag & 0xE000;
}

const uint16_t Packet::getFragmentID() const {
    return ip_.ipFrag & 0x1FFF;  // mask out first 3 bits 
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


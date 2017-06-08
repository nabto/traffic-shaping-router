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
#include <netinet/ip.h>
#include <linux/tcp.h>



Packet::Packet(struct nfq_data *nfa) : stamp_(boost::posix_time::microsec_clock::local_time()) {
    struct nfq_data* nfData = nfa;
    u_int32_t ifi;
    char buf[IF_NAMESIZE];
    rawPacket* orgPktDataPtr;
    
    struct nfqnl_msg_packet_hdr *orgPktHead = nfq_get_msg_packet_hdr(nfData);
    nfqID_ = ntohl(orgPktHead->packet_id);
    packetDataLen_ = nfq_get_payload(nfData, (uint8_t**)&orgPktDataPtr);
    packetData_.clear();
//    packetData_.insert(packetData_.begin(),(uint8_t*)orgPktDataPtr,packetDataLen_);
    std::copy((uint8_t*)orgPktDataPtr, (uint8_t*)orgPktDataPtr + packetDataLen_, std::back_inserter(packetData_));
    
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
    ipLen_ = ntohs(ip->tot_len);
    ipTos_ = ip->tos;
    ipId_ = ntohs(ip->id);
    ipFrag_ = ntohs(ip->frag_off);
    ipTtl_ = ip->ttl;
    ipProt_ = ip->protocol;
    ipSrc_ = ntohl(ip->saddr);
    ipDst_ = ntohl(ip->daddr);
    ipHdrLen_ = ip->ihl << 2;
    
    if (ipProt_ == PROTO_UDP){
        udphdr* udp = (udphdr*)(packetData_.data()+ipHdrLen_);
        sport_ = ntohs(udp->uh_sport);
        dport_ = ntohs(udp->uh_dport);
        transLen_ = ntohs(udp->uh_ulen);
    } else if (ipProt_ == PROTO_TCP){
#ifdef TRACE_LOG
        std::cout << "Dumping entire TCP packet" << std::endl;
        dumpMem((uint8_t*)&packetData_, packetDataLen_);
#endif
        tcphdr* tcp = (tcphdr *)((packetData_.data()+ipHdrLen_));
        transLen_ = packetDataLen_;

        urg_ = ntohs(tcp->urg_ptr);
        win_ = ntohs(tcp->window);
        control_ = tcp->fin | (tcp->syn << 1) | (tcp->rst << 2) | (tcp->psh << 3) | (tcp->ack << 4) | (tcp->urg << 5) | (tcp->ece << 6) | (tcp->cwr << 7);
        ack_ = ntohl(tcp->ack_seq);
        seq_ = ntohl(tcp->seq);
        dport_ = ntohs(tcp->dest);
        sport_ = ntohs(tcp->source);
        tcpOptionsSize_ = (tcp->doff << 2) - 20;
        
        tcpPayloadSize_ = packetDataLen_ - ipHdrLen_ - 20 - tcpOptionsSize_;
        tcpPayload_ = (uint8_t*)tcp + 20 + tcpOptionsSize_;

        tcpOptions_ = (uint8_t*)tcp + 20;

#ifdef TRACE_LOG
        std::cout << "TCP DATA:\n\ttcpPayloadSize_ " << tcpPayloadSize_ << "\n\tlen: " << transLen_ << "\n\turg: " << urg_ << "\n\twin: " << win_ << "\n\tcontrol: " << unsigned(control_) << "\n\tack: " << ack_  << "\n\tseq: " << seq_ << "\n\tdport: " << dport_ << "\n\tsport: " << sport_  << "\n\tOptSize: " << unsigned(tcpOptionsSize_) << "\n\tOpt: ";
        for (int i = 0; i<tcpOptionsSize_; i++){
            std::cout << unsigned(tcpOptions_[i]) << " ";
        }
        std::cout << std::endl;
#endif
    } else if (ipProt_ == PROTO_ICMP){
        transLen_ = ipLen_ - ipHdrLen_;
    } else {
        // unknown protocol passing it raw, but NAT should drop it
        transLen_ = ipLen_ - ipHdrLen_;
    }
}

// Dummy packet for testing purposes
Packet::Packet() : stamp_(boost::posix_time::microsec_clock::local_time()) {
    ipLen_ = 5;
    ipTos_ = 0;
    ipId_ = 3;
    ipFrag_ = 5;
    ipTtl_ = 55;
    ipProt_ = PROTO_TCP;
    ipSrc_ = 33685514;
    ipDst_ = 33559212;
    ipHdrLen_ = 20;
    nfqID_ = 5;
    strInboundInterface_ = "eth8";
    strOutboundInterface_ = "eth9";
}

Packet::~Packet() {
}

ROUTER_STATUS Packet::send() {
    ROUTER_STATUS ret = E_UNDEFINED;
    libnet_t *l;
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_ptag_t ip_ptag = 0;
    int count;

    l = libnet_init(LIBNET_RAW4,(char*)strOutboundInterface_.c_str(),errbuf);
    
    if (l == NULL) {
        fprintf(stderr, "libnet_init() failed: %s", errbuf);
        ret = E_FAILED;
    } else {
        ret = S_OK;
    }

    if (ret == S_OK) {
        // Build packet
        // Advance pointer past any options headers.
        unsigned char* pData = (packetData_.data()+ipHdrLen_);
        uint32_t size = transLen_;
        if (this->getProtocol() == PROTO_ICMP) {
        } else if (this->getProtocol() == PROTO_UDP) {
            int checkerr = libnet_build_udp(
                sport_,
                dport_,
                transLen_,
                0,
                packetData_.data()+ipHdrLen_+8,
                transLen_- 8,
                l,
                0);
            if (checkerr == -1) {
                std::cout << "Error building UDP header: " << libnet_geterror(l) << std::endl;
                libnet_destroy(l);
                return E_FAILED;
            }            
            pData = NULL;
            size = 0;

        } else if (this->getProtocol() == PROTO_TCP) {
            // build tcp header 

            if (tcpPayloadSize_ == 0){
                tcpPayload_ = NULL;
            }
            
#ifdef TRACE_LOG
            std::cout << "dumpint tcp options: " << std::endl;
            dumpMem((uint8_t *)tcpOptions_, tcpOptionsSize_);
#endif
            int checkerr = libnet_build_tcp_options(
                tcpOptions_,
                tcpOptionsSize_,
                l,
                0);

            if (checkerr == -1) {
                std::cout << "Error building TCP Options: " << libnet_geterror(l) << std::endl;
                libnet_destroy(l);
                return E_FAILED;
            }

            checkerr = libnet_build_tcp(
                sport_,
                dport_,
                seq_,
                ack_,
                control_,
                win_,
                0,
                urg_,
                ipHdrLen_+20+tcpOptionsSize_+tcpPayloadSize_,//len_,
                tcpPayload_,
                tcpPayloadSize_,
                l,
                0);
            if (checkerr == -1) {
                std::cout << "Error building TCP header: " << libnet_geterror(l) << std::endl;
                libnet_destroy(l);
                return E_FAILED;
            }

            
            pData = NULL;
            size = 0;
            
        } else {
            std::cout << "\tUnknown Protocol" << std::endl;
            libnet_destroy(l);
            return E_FAILED;
        }

#ifdef TRACE_LOG
        std::cout << "ip size: " << size << " sending to ip " << ipDst_ << " from " << ipSrc_ << std::endl;
#endif
        ip_ptag = libnet_build_ipv4(
            ipLen_, /* length */
            ipTos_,	  /* TOS */
            this->getFragmentID(),    /* IP fragment ID */
            this->getFragmentFlags(), /* IP Frag flags*/
            ipTtl_,        /* TTL */
            ipProt_,  /* protocol */
            0,                        /* checksum (let libnet calculate) */
            htonl(ipSrc_),  /* source IP */
            htonl(ipDst_),  /* destination IP */
            pData,                    /* payload */
            size, /* payload size */
            l,                        /* libnet handle */
		    ip_ptag);                 /* libnet id */


        if (ip_ptag == -1) {
            std::cout << "Cant't build IP header" << libnet_geterror(l)  << std::endl;
            ret = E_FAILED;
        } else {
            ret = S_OK;
        }

        // TODO: Add IP options (if any)

        if (ret == S_OK) {
            // Write to network
            count = libnet_write(l);
            if (count == -1) {
                std::cout << "Failed to write packet: " << libnet_geterror(l) << std::endl;
                ret = E_FAILED;
            } else {
#ifdef TRACE_LOG
                std::cout << "Wrote " << count << " bytes with libnet" << std::endl;
#endif
                ret = S_OK;
            }
        }

        libnet_destroy(l);
    }
    return ret;
}

void Packet::dump() {
    printf("\tid is %i\n", getNetfilterID());
    printf("\tInbound interface: %s\n",strInboundInterface_.c_str());
    printf("\tOutbound interface: %s\n",strOutboundInterface_.c_str());
    printf("\ttime stamp was: %s\n", boost::posix_time::to_simple_string(stamp_).c_str());

    // TODO: these two printfs will break if Network order != Host order
    printf("\tSource IP raw: %u\n", ipSrc_);
    printf("\tSource IP: %u.%u.%u.%u\n",
           ipSrc_ >> 24,
           (ipSrc_ >> 16) & 0xFF,
           (ipSrc_ >> 8) & 0xFF,
           ipSrc_ & 0xFF);

    printf("\tDestination IP raw: %u\n", ipDst_);
    printf("\tDestination IP: %u.%u.%u.%u\n",
           ipDst_ >> 24,
           (ipDst_ >> 16) & 0xFF,
           (ipDst_ >> 8) & 0xFF,
           ipDst_ & 0xFF);

    if (this->getProtocol() == PROTO_ICMP) {
        printf("\tICMP Packet\n");
    } else if (this->getProtocol() == PROTO_UDP) {
        printf("\tUDP Source Port: %u\n",sport_);
        printf("\tUDP Destination Port: %u\n",dport_);
        printf("\tHeader len: %u\n",transLen_); 

    } else if (this->getProtocol() == PROTO_TCP) {
        printf("\tTCP Source Port: %d\n",sport_);
        printf("\tTCP Destination Port: %d\n",dport_);
        printf("\tHeader seq: %u\n",seq_); 
        printf("\tHeader ack: %u\n",ack_); 
        printf("\tHeader win: %u\n",win_); 

    } else {
        printf("\tUnknown Protocol\n");
    }
}


const int Packet::getNetfilterID() const {
    return nfqID_;
}

const uint8_t Packet::getProtocol() const {
    return ipProt_;
}

const uint32_t Packet::getSourceIP() const {
    return ipSrc_;
}

void Packet::setSourceIP(uint32_t ip) {
    ipSrc_ = ip;
}

void Packet::setDestinationIP(uint32_t ip) {
    ipDst_ = ip;
}

const uint32_t Packet::getDestinationIP() const {
    return ipDst_;
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
    return ipFrag_ & 0xE000;
}

const uint16_t Packet::getFragmentID() const {
    return ipFrag_ & 0x1FFF;  // mask out first 3 bits 
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


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
//#include <netinet/tcp.h>
#include <linux/tcp.h>



Packet::Packet(struct nfq_data *nfa) : stamp_(boost::posix_time::microsec_clock::local_time()) {
    struct nfq_data* nfData = nfa;
    u_int32_t ifi;
    char buf[IF_NAMESIZE];
    rawPacket* orgPktDataPtr;
    
    struct nfqnl_msg_packet_hdr *orgPktHead = nfq_get_msg_packet_hdr(nfData);
    std::memcpy(&ph_,orgPktHead,sizeof(struct nfqnl_msg_packet_hdr));
    packetDataLen_ = nfq_get_payload(nfData, (uint8_t**)&orgPktDataPtr);
    std::memcpy(&packetData_, orgPktDataPtr, packetDataLen_);
    
    
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
    uint16_t * ptr = (uint16_t*)((packetData_.data));
    sport_ = ntohs(*ptr);
    dport_ = ntohs(*(ptr+1));

    if (getProtocol() == PROTO_TCP){
#ifdef TRACE_LOG
        std::cout << "Dumping entire TCP packet" << std::endl;
        dumpMem((uint8_t*)&packetData_, packetDataLen_);
#endif
        tcphdr* tcp = (tcphdr *)((packetData_.data));
        len_ = packetDataLen_;

        urg_ = ntohs(tcp->urg_ptr);
        tcpSum_ = 0;
        win_ = ntohs(tcp->window);
        control_ = tcp->fin | (tcp->syn << 1) | (tcp->rst << 2) | (tcp->psh << 3) | (tcp->ack << 4) | (tcp->urg << 5) | (tcp->ece << 6) | (tcp->cwr << 7);
        ack_ = ntohl(tcp->ack_seq);
        seq_ = ntohl(tcp->seq);
        dport_ = ntohs(tcp->dest);
        sport_ = ntohs(tcp->source);
        tcpOptionsSize_ = (tcp->doff << 2) - 20;
        
        tcpPayloadSize_ = packetDataLen_ - getPacketHeaderLength() - 20 - tcpOptionsSize_;
        tcpPayload_ = (uint8_t*)&packetData_ +  getPacketHeaderLength() + 20 + tcpOptionsSize_;

        tcpOptions_ = (uint8_t*)&packetData_ +  getPacketHeaderLength() + 20;

#ifdef TRACE_LOG
        std::cout << "TCP DATA:\n\ttcpPayloadSize_ " << tcpPayloadSize_ << "\n\tlen: " << len_ << "\n\turg: " << urg_ << "\n\tsum: " << tcpSum_ << "\n\twin: " << win_ << "\n\tcontrol: " << unsigned(control_) << "\n\tack: " << ack_  << "\n\tseq: " << seq_ << "\n\tdport: " << dport_ << "\n\tsport: " << sport_  << "\n\tOptSize: " << unsigned(tcpOptionsSize_) << "\n\tOpt: ";
        for (int i = 0; i<tcpOptionsSize_; i++){
            std::cout << unsigned(tcpOptions_[i]) << " ";
        }
        std::cout << std::endl;
#endif
    }

}

// Dummy packet for testing purposes
Packet::Packet() : stamp_(boost::posix_time::microsec_clock::local_time()) {
    packetData_.nVersionLength = 2;
    packetData_.flagsTOS = 0;
    packetData_.nPacketLength = 100;
    packetData_.nFragmentID = 3;
    packetData_.nFragFlagsOffset = 0;
    packetData_.TTL = 55;
    packetData_.nProtocol = PROTO_TCP;
    packetData_.nHeaderChecksum = 0;
    packetData_.srcIP.raw = 202;
    packetData_.srcIP.octet[0] = 65;
    packetData_.srcIP.octet[1] = 65;
    packetData_.srcIP.octet[2] = 65;
    packetData_.srcIP.octet[3] = 65;
    packetData_.dstIP.raw = 202;
    packetData_.dstIP.octet[0] = 65;
    packetData_.dstIP.octet[1] = 65;
    packetData_.dstIP.octet[2] = 65;
    packetData_.dstIP.octet[3] = 65;
    ph_.packet_id = 5;
    strInboundInterface_ = "eth8";
    strOutboundInterface_ = "eth9";
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
        unsigned char* pData = (packetData_.data);
        uint32_t size = ntohs(packetData_.nPacketLength) - this->getPacketHeaderLength();
        if (this->getProtocol() == PROTO_ICMP) {
        } else if (this->getProtocol() == PROTO_UDP) {
            //udphdr* p2 = (udphdr *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);
            int checkerr = libnet_build_udp(
                sport_,
                dport_,
                packetDataLen_-getPacketHeaderLength(),
                0,
                pData+8,
                packetDataLen_-getPacketHeaderLength()- 8,
                l,
                0);
            if (checkerr == -1) {
                printf("Error building UDP header: %s\n", libnet_geterror(l));
                libnet_destroy(l);
                return E_FAILED;
            }            
            size = 0;
            pData = NULL;

        } else if (this->getProtocol() == PROTO_TCP) {
            //tcphdr* tcp = (tcphdr *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);
            size = ntohs(packetData_.nPacketLength)- getPacketHeaderLength()-20;
            // build tcp header 

            if (tcpPayloadSize_ == 0){
                tcpPayload_ = NULL;
            }
            
#ifdef TRACE_LOG
            std::cout << "TCP hdr len: " << size << std::endl;
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
                getPacketHeaderLength()+20+tcpOptionsSize_+tcpPayloadSize_,//len_,
                tcpPayload_,
                tcpPayloadSize_,
                l,
                0);
            if (checkerr == -1) {
                std::cout << "Error building TCP header: " << libnet_geterror(l) << std::endl;
                libnet_destroy(l);
                return E_FAILED;
            }

            
            pData = NULL; //((uint8_t *)tcp)+20;
            size = 0;
            
        } else {
            std::cout << "\tUnknown Protocol" << std::endl;
            libnet_destroy(l);
            return E_FAILED;
        }

#ifdef TRACE_LOG
        std::cout << "ip size: " << size << " sending to ip " << ntohl(packetData_.dstIP.raw) << " from " << ntohl(packetData_.srcIP.raw) << std::endl;
#endif
        ip_ptag = libnet_build_ipv4(
            LIBNET_IPV4_H + ntohs(packetData_.nPacketLength) - this->getPacketHeaderLength(), /* length */
            packetData_.flagsTOS,	  /* TOS */
            this->getFragmentID(),    /* IP fragment ID */
            this->getFragmentFlags(), /* IP Frag flags*/
            packetData_.TTL,        /* TTL */
            packetData_.nProtocol,  /* protocol */
            0,                        /* checksum (let libnet calculate) */
            packetData_.srcIP.raw,  /* source IP */
            packetData_.dstIP.raw,  /* destination IP */
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
    printf("\tSource IP raw: %u\n", packetData_.srcIP.raw);
    printf("\tSource IP: %d.%d.%d.%d\n",
           packetData_.srcIP.octet[0],
           packetData_.srcIP.octet[1],
           packetData_.srcIP.octet[2],
           packetData_.srcIP.octet[3]);

    printf("\tDestination IP raw: %u\n", packetData_.dstIP.raw);
    printf("\tDestination IP: %d.%d.%d.%d\n",
           packetData_.dstIP.octet[0],
           packetData_.dstIP.octet[1],
           packetData_.dstIP.octet[2],
           packetData_.dstIP.octet[3]);

    if (this->getProtocol() == PROTO_ICMP) {
        printf("\tICMP Packet\n");
    } else if (this->getProtocol() == PROTO_UDP) {
        uint16_t* pData = (uint16_t *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);

        printf("\tUDP Source Port: %d\n",ntohs(*pData));
        printf("\tUDP Destination Port: %d\n",ntohs(*(pData+1)));

        udphdr* p2 = (udphdr *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);
        
        printf("\tHeader srcPort: %d\n",ntohs(p2->uh_sport)); 
        printf("\tHeader dstPort: %d\n",ntohs(p2->uh_dport)); 
        printf("\tHeader len: %d\n",ntohs(p2->uh_ulen)); 
        printf("\tHeader sum: %d\n",ntohs(p2->uh_sum)); 

    } else if (this->getProtocol() == PROTO_TCP) {
        uint16_t* pData = (uint16_t *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);

        printf("\tTCP Source Port: %d\n",ntohs(*pData));
        printf("\tTCP Destination Port: %d\n",ntohs(*(pData+1)));

        // tcphdr* p2 = (tcphdr *)((packetData_.data) + this->getPacketHeaderLength() - LIBNET_IPV4_H);
        
        // printf("\tHeader srcPort: %d\n",ntohs(p2->th_sport)); 
        // printf("\tHeader dstPort: %d\n",ntohs(p2->th_dport)); 
        // printf("\tHeader seq: %d\n",ntohs(p2->th_seq)); 
        // printf("\tHeader ack: %d\n",ntohs(p2->th_ack)); 
        // printf("\tHeader win: %d\n",ntohs(p2->th_win)); 
        // printf("\tHeader sum: %d\n",ntohs(p2->th_sum)); 

    } else {
        printf("\tUnknown Protocol\n");
    }
}


const uint8_t Packet::getProtocol() const {
    return packetData_.nProtocol;
}

const uint32_t Packet::getSourceIP() const {
    return ntohl(packetData_.srcIP.raw);
}

void Packet::setSourceIP(uint32_t ip) {
    packetData_.srcIP.raw = ip;
    packetData_.srcIP.octet[0] = ip >> 24;
    packetData_.srcIP.octet[1] = (ip >> 16) & 0xFF;
    packetData_.srcIP.octet[2] = (ip >> 8) & 0xFF;
    packetData_.srcIP.octet[3] = ip & 0xFF;
    
}

void Packet::setDestinationIP(uint32_t ip) {
    packetData_.dstIP.raw = ip;
    packetData_.dstIP.octet[0] = ip >> 24;
    packetData_.dstIP.octet[1] = (ip >> 16) & 0xFF;
    packetData_.dstIP.octet[2] = (ip >> 8) & 0xFF;
    packetData_.dstIP.octet[3] = ip & 0xFF;
    
}

const uint32_t Packet::getDestinationIP() const {
    return ntohl(packetData_.dstIP.raw);
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
    uint16_t data = ntohs(packetData_.nFragFlagsOffset);
    return data & 0xE000;
}

const uint16_t Packet::getFragmentID() const {
    uint16_t data = ntohs(packetData_.nFragFlagsOffset);
    return data & 0x1FFF;  // mask out first 3 bits 
}

inline short Packet::getPacketHeaderLength() const {
    return (packetData_.nVersionLength & 0x0F) << 2;
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


#include "output.hpp"

#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

Output::Output(): queue_(*(TpService::getInstance()->getIoService())) {
    l_ = libnet_init(LIBNET_RAW4, "",errbuf_);
}

Output::~Output() {
    libnet_destroy(l_);
}
    
bool Output::init() {
    queue_.asyncPop(std::bind(&Output::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    return true;
}

void Output::handlePacket(PacketPtr pkt) {
    queue_.push(pkt);
}

void Output::popHandler(const std::error_code& ec, const PacketPtr pkt) {
    if(ec == queue_error_code::stopped){
        std::cout << "Output pophandler got AsyncQueue Stopped" << std::endl;
        return;
    }
    if(l_ == NULL){
        std::cout << "Libnet_init() Failed: " << errbuf_ << std::endl <<  "\tretrying: " << std::endl;
        l_ = libnet_init(LIBNET_RAW4,"",errbuf_);
        if (l_ == NULL){
            std::cout << "\t failed, giving up: " << errbuf_ << std::endl;
            return;
        }
    }
    libnet_clear_packet(l_);
#ifdef TRACE_LOG
        std::cout << "Output dumping packet: " << std::endl;
        pkt->dump();
#endif

    std::vector<uint8_t> pktData = pkt->getPacketData();

    iphdr* ip = (iphdr*)pktData.data();
    uint8_t ipHdrLen = ip->ihl << 2;

    unsigned char* pData = (pktData.data()+ipHdrLen);
    uint32_t size;
    libnet_ptag_t ip_ptag = 0; 
    if (ip->protocol == PROTO_ICMP){
        size = ntohs(ip->tot_len) - ipHdrLen;
    } else if (ip->protocol == PROTO_UDP) {
        udphdr* udp = (udphdr*)(pktData.data()+ipHdrLen);
        int checkerr = libnet_build_udp(
            pkt->getSourcePort(),
            pkt->getDestinationPort(),
            ntohs(udp->uh_ulen),
            0,
            pktData.data()+ipHdrLen+8,
            ntohs(udp->uh_ulen) - 8,
            l_,
            0);
        if (checkerr == -1) {
            std::cout << "Error building UDP header: " << libnet_geterror(l_) << std::endl;
            return;
        }            
        pData = NULL;
        size = 0;
    } else if (ip->protocol == PROTO_TCP) {
        // build tcp header 
        tcphdr* tcp = (tcphdr*)(pktData.data()+ipHdrLen);
        uint8_t tcpOptionsSize = (tcp->th_off << 2) - 20;
        uint32_t tcpPayloadSize = pktData.size() - ipHdrLen - 20 - tcpOptionsSize;;
        const uint8_t * tcpPayload = (uint8_t*)tcp + 20 + tcpOptionsSize;
        
        if (tcpPayloadSize == 0){
            tcpPayload = NULL;
        }
            
#ifdef TRACE_LOG
        std::cout << "dumpint tcp options: " << std::endl;
        dumpMem((uint8_t *)tcp + 20, tcpOptionsSize);
#endif
        int checkerr = libnet_build_tcp_options(
            (uint8_t*)tcp + 20,
            tcpOptionsSize,
            l_,
            0);

        if (checkerr == -1) {
            std::cout << "Error building TCP Options: " << libnet_geterror(l_) << std::endl;
            return;
        }

        checkerr = libnet_build_tcp(
            pkt->getSourcePort(),
            pkt->getDestinationPort(),
            ntohl(tcp->th_seq),
            ntohl(tcp->th_ack),
            tcp->th_flags,
            ntohs(tcp->th_win),
            0,
            ntohs(tcp->th_urp),
            ipHdrLen+20+tcpOptionsSize+tcpPayloadSize,
            tcpPayload,
            tcpPayloadSize,
            l_,
            0);
        if (checkerr == -1) {
            std::cout << "Error building TCP header: " << libnet_geterror(l_) << std::endl;
            return;
        }

        pData = NULL;
        size = 0;
    } else {
        std::cout << "\tUnknown Protocol" << std::endl;
        size = ntohs(ip->tot_len) - ipHdrLen;
    }

#ifdef TRACE_LOG
    std::cout << "ip size: " << size << " sending to ip " << pkt->getDestinationIP() << " from " << pkt->getSourceIP() << std::endl;
#endif
    ip_ptag = libnet_build_ipv4(
        ntohs(ip->tot_len),      // length
        ip->tos,                 // TOS
        pkt->getFragmentID(),    // IP fragment ID
        pkt->getFragmentFlags(), // IP Frag flags
        ip->ttl,                 // TTL
        ip->protocol,            // protocol
        0,                       // checksum (let libnet calculate)
        htonl(pkt->getSourceIP()),      // source IP
        htonl(pkt->getDestinationIP()), // destination IP
        pData,                   // payload
        size,                    // payload size
        l_,                      // libnet handle
        ip_ptag);                // libnet id


    if (ip_ptag == -1) {
        std::cout << "Cant't build IP header" << libnet_geterror(l_)  << std::endl;
        return;
    }

    // Write to network
    int count = libnet_write(l_);
    if (count == -1) {
        std::cout << "Failed to write packet: " << libnet_geterror(l_) << std::endl;
        return;
    } else {
#ifdef TRACE_LOG
        std::cout << "Wrote " << count << " bytes with libnet" << std::endl;
#endif
    }
    queue_.asyncPop(std::bind(&Output::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Output::dumpMem(unsigned char* p,int len) {
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

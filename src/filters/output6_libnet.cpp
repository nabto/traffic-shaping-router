#include "output6_libnet.hpp"

#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip6.h>


Output6Libnet::Output6Libnet(): queue_(*(TpService::getInstance()->getIoService())) {
    l_ = libnet_init(LIBNET_RAW6, NULL, errbuf_);
}

Output6Libnet::~Output6Libnet() {
    libnet_destroy(l_);
}
    
bool Output6Libnet::init() {
    queue_.asyncPop(std::bind(&Output6Libnet::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    return true;
}

void Output6Libnet::handlePacket(std::shared_ptr<ParentPacket> parPkt) {
    Packet6Ptr pkt = std::dynamic_pointer_cast<Packet6>(parPkt);
    queue_.push(pkt);
}

void Output6Libnet::popHandler(const std::error_code& ec, const Packet6Ptr pkt) {
    if(ec == queue_error_code::stopped){
        std::cout << "Output6Libnet pophandler got AsyncQueue Stopped" << std::endl;
        return;
    }
    if(l_ == NULL){
        std::cout << "Libnet_init() Failed: " << errbuf_ << std::endl <<  "\tretrying: " << std::endl;
        l_ = libnet_init(LIBNET_RAW6, NULL, errbuf_);
        if (l_ == NULL){
            std::cout << "\t failed, giving up: " << errbuf_ << std::endl;
            return;
        }
    }
    libnet_clear_packet(l_);
#ifdef TRACE_LOG
        std::cout << "Output6Libnet dumping packet: " << std::endl;
        pkt->dump();
#endif

    std::vector<uint8_t> pktData = pkt->getPacketData();

    unsigned char* pData = (pktData.data()+40);
    uint32_t size = pkt->getPayloadLen();//pktData.size()-40;
    libnet_ptag_t ip_ptag = 0; 
/*    if (pkt->getNextHeader() == IPPROTO_ICMP){
        size = pkt->getPayloadLen();
    } else if (pkt->getNextHeader() == IPPROTO_ICMPV6){
        size = pkt->getPayloadLen();
    } else if (pkt->getNextHeader() == IPPROTO_UDP) {
        udphdr* udp = (udphdr*)pData;
        int checkerr = libnet_build_udp(
            pkt->getSourcePort(),
            pkt->getDestinationPort(),
            ntohs(udp->uh_ulen),
            0,
            pktData.data()+40+8,
            ntohs(udp->uh_ulen) - 8,
            l_,
            0);
        if (checkerr == -1) {
            std::cout << "Error building UDP header: " << libnet_geterror(l_) << std::endl;
            return;
        }            
        pData = NULL;
        size = 0;
    } else if (pkt->getNextHeader() == IPPROTO_TCP) {
        // build tcp header 
        tcphdr* tcp = (tcphdr*)pData;
        uint8_t tcpOptionsSize = (tcp->th_off << 2) - 20;
        uint32_t tcpPayloadSize = pktData.size() - 40 - 20 - tcpOptionsSize;;
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
            40+20+tcpOptionsSize+tcpPayloadSize,
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
        std::cout << "\tUnknown Protocol: " << pkt->getNextHeader() << std::endl;
        size = pkt->getPayloadLen();
    }
*/
#ifdef TRACE_LOG
    std::cout << "ip size: " << size << " sending packet:" << std::endl;
    pkt->dump();
#endif
    struct in6_addr src = pkt->getSourceIP();
    libnet_in6_addr* libnetSrc = (libnet_in6_addr*)&src;
    struct in6_addr dst = pkt->getDestinationIP();
    libnet_in6_addr* libnetDst = (libnet_in6_addr*)&dst;
    ip_ptag = libnet_build_ipv6(
        pkt->getTclass(),        // Tclass
        pkt->getFlowLabel(),     // Flow Label
        size,          // Total length of IP packet
        pkt->getNextHeader(),    // Next header
        pkt->getHopLimit(),      // Hop limit
        *libnetSrc,              // Source IP
        *libnetDst,              // Destination IP
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
    queue_.asyncPop(std::bind(&Output6Libnet::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Output6Libnet::dumpMem(unsigned char* p,int len) {
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

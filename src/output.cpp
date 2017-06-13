#include "output.hpp"

//#include <netinet/udp.h>
//#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
//#include <linux/tcp.h>

//#define TRACE_LOG

void Output::popHandler(const boost::system::error_code& ec, const PacketPtr pkt) {
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

    struct transParams trans = pkt->getTransParams();
    std::vector<uint8_t> pktData = pkt->getPacketData();

    iphdr* ip = (iphdr*)pktData.data();
    uint8_t ipHdrLen = ip->ihl << 2;

    unsigned char* pData = (pktData.data()+ipHdrLen);
    uint32_t size = 0;
    libnet_ptag_t ip_ptag = 0; 
    if (ip->protocol == PROTO_ICMP){
        size = trans.len;
    } else if (ip->protocol == PROTO_UDP) {
        int checkerr = libnet_build_udp(
            trans.sport,
            trans.dport,
            trans.len,
            0,
            pktData.data()+ipHdrLen+8,
            trans.len - 8,
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
        struct tcpParams tcp = pkt->getTcpParams();
        if (tcp.tcpPayloadSize == 0){
            tcp.tcpPayload = NULL;
        }
            
#ifdef TRACE_LOG
        std::cout << "dumpint tcp options: " << std::endl;
        dumpMem((uint8_t *)tcp.tcpOptions, tcp.tcpOptionsSize);
#endif
        int checkerr = libnet_build_tcp_options(
            tcp.tcpOptions,
            tcp.tcpOptionsSize,
            l_,
            0);

        if (checkerr == -1) {
            std::cout << "Error building TCP Options: " << libnet_geterror(l_) << std::endl;
            return;
        }

        checkerr = libnet_build_tcp(
            trans.sport,
            trans.dport,
            tcp.seq,
            tcp.ack,
            tcp.control,
            tcp.win,
            0,
            tcp.urg,
            ipHdrLen+20+tcp.tcpOptionsSize+tcp.tcpPayloadSize,
            tcp.tcpPayload,
            tcp.tcpPayloadSize,
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
        size = trans.len;
    }

#ifdef TRACE_LOG
    std::cout << "ip size: " << size << " sending to ip " << ntohl(ip->daddr) << " from " << ntohl(ip->saddr) << std::endl;
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
    //pkt->send();
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

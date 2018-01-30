#ifndef PARENT_PACKET_H // one-time include
#define PARENT_PACKET_H

#include <stdint.h>

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
}
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <functional>

#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/icmp6.h>

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <linux/netfilter.h>
}

class ParentPacket
{
 public:
    ParentPacket() {verdictSet_ = false;}
    virtual void setOutboundInterface(const std::string & out) {strOutboundInterface_ = out;}
    // Get functions for packet information
    virtual const int getNetfilterID() const {return nfqID_;}
    virtual struct nfq_q_handle * getQH(){return qh_;}
    virtual const uint32_t getPacketLen() const {return (uint32_t)packetDataLen_;}
    virtual const std::vector<uint8_t> getPacketData() const {return packetData_;}
    virtual void getInboundInterface(std::string & in) const {in = strInboundInterface_;}
    virtual void getOutboundInterface(std::string & out) const {out = strOutboundInterface_;}
    virtual void setVerdict(bool accept) {
        if (verdictSet_ == true) {
            return;
        }
        if (accept) {
            nfq_set_verdict(getQH(), getNetfilterID(), NF_ACCEPT, 0, NULL);
            verdictSet_ = true;
        } else {
            nfq_set_verdict(getQH(), getNetfilterID(), NF_DROP, 0, NULL);
            verdictSet_ = true;
        }
    }

    // Function dumping packet data to stdout for debugging
    virtual void dump() {}
    // Timestamp is first set in the constructor, this can be retrieved and reset here
    virtual void resetTimeStamp(){stamp_ = boost::posix_time::microsec_clock::local_time();}
    virtual boost::posix_time::ptime getTimeStamp(){return stamp_;}
    virtual void setIngoing(bool i) {isIngoing_ = i;}
    virtual bool isIngoing() {return isIngoing_;}

    virtual void setSourcePort(uint16_t sport){sport_ = sport;}
    virtual void setDestinationPort(uint16_t dport){dport_ = dport;}
    virtual const uint16_t getSourcePort() const {return sport_;}
    virtual const uint16_t getDestinationPort() const {return dport_;}
    virtual const uint16_t getIcmpId() const {return icmpId_;}
    
 protected:
    boost::posix_time::ptime stamp_;
    bool isIngoing_;
    bool verdictSet_;

    struct nfq_q_handle *qh_;
    int nfqID_;
    int packetDataLen_;
    std::vector<uint8_t> packetData_;
    std::string	strInboundInterface_;
    std::string	strOutboundInterface_;

    uint16_t sport_;
    uint16_t dport_;
    uint16_t icmpId_;

    void dumpMem(uint8_t* p, int len) {
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
    void decodeTransportProtocol(uint8_t transProt, uint8_t* transHdr) {
        if (transProt == IPPROTO_UDP){
            udphdr* udp = (udphdr*)transHdr;
            sport_ = ntohs(udp->uh_sport);
            dport_ = ntohs(udp->uh_dport);
        } else if (transProt == IPPROTO_TCP){
#ifdef TRACE_LOG
            std::cout << "Dumping entire TCP packet" << std::endl;
            dumpMem((uint8_t*)&packetData_, packetData_.size());
#endif
            tcphdr* tcp = (tcphdr *)transHdr;
            sport_ = ntohs(tcp->th_sport);
            dport_ = ntohs(tcp->th_dport);
#ifdef TRACE_LOG
            std::cout << "TCP Packet with:\n\tdport: " << dport_ << "\n\tsport: " << sport_ << std::endl;
#endif
        } else if (transProt == IPPROTO_ICMP){
            icmphdr* icmp = (icmphdr*)transHdr;
            icmpId_ = icmp->un.echo.id;
            sport_ = icmpId_;
            dport_ = icmpId_;
        } else if (transProt == IPPROTO_ICMPV6){
            icmp6_hdr* icmp = (icmp6_hdr*)transHdr;
            icmpId_ = icmp->icmp6_id;
            sport_ = icmpId_;
            dport_ = icmpId_;
        }
    }
};

#endif

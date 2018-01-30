#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <filter.hpp>
#include <parent_packet.hpp>
extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <linux/netfilter.h>
}

// The output filter is the last filter, which sends the packet on the network
class Output : public Filter, public std::enable_shared_from_this<Output>
{
 public:
    Output(){}
    ~Output(){}
    bool init(){return true;}
    void handlePacket(std::shared_ptr<ParentPacket> pkt){
        pkt->setVerdict(true);
    }
    void setQH(struct nfq_q_handle *qh){qh_ = qh; std::cout << "setQH called in Output" << std::endl;}

 private:
    struct nfq_q_handle *qh_;
};


#endif //OUTPUT6_HPP

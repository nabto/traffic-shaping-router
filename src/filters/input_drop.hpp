#ifndef INPUTDROP_HPP
#define INPUTDROP_HPP

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <linux/netfilter.h>
}

class InputDrop : public Filter, public std::enable_shared_from_this<InputDrop>
{
 public:
    InputDrop() {}
    ~InputDrop() {}
    void handlePacket(std::shared_ptr<ParentPacket> pkt){
        pkt->setVerdict(false);
        next_->handlePacket(pkt);
    }
    void setQH(struct nfq_q_handle *qh){qh_ = qh;}

 private:
    struct nfq_q_handle *qh_;
};

#endif //INPUTDROP_HPP

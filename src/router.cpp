
#include <stddef.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <mutex>
#include <stdint.h>
#include <functional>
#include <errno.h>

#include "router.hpp"

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
}

#define RECV_BUF_SIZE 65536

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    RouterPtr rt = Router::getInstance();
    return rt->newPacket(qh,nfmsg,nfa,data);
}

Router::Router() {
    initialized_ = false;
}

void Router::init() {
    std::cout << "using all filters" << std::endl;
    loss_ = std::make_shared<Loss>();
    nat_ = std::make_shared<Nat>();
    burst_ = std::make_shared<Burst>();
    delay_ = std::make_shared<StaticDelay>();
    tbf_ = std::make_shared<TokenBucketFilter>();
    output_ = std::make_shared<Output>();
    setNext(loss_);
    loss_->setNext(nat_);
    nat_->setNext(burst_);
    burst_->setNext(delay_);
    delay_->setNext(tbf_);
    tbf_->setNext(output_);
    
    output_->init();
    delay_->init();
    burst_->run();
    tbf_->run();
    initialized_ = true;
}

void Router::init(std::vector<std::string> filters) {
    std::shared_ptr<Filter> last = shared_from_this();
    for (auto it : filters){
        if (it.compare("Burst") == 0){
#ifdef TRACE_LOG
            std::cout << "adding burst filter" << std::endl;
#endif
            burst_ = std::make_shared<Burst>();
            last->setNext(burst_);
            last = burst_;
            burst_->run();
        } else if (it.compare("Loss") == 0){
#ifdef TRACE_LOG
            std::cout << "adding Loss filter" << std::endl;
#endif
            loss_ = std::make_shared<Loss>();
            last->setNext(loss_);
            last = loss_;
        } else if (it.compare("StaticDelay") == 0){
#ifdef TRACE_LOG
            std::cout << "adding StaticDelay filter" << std::endl;
#endif
            delay_ = std::make_shared<StaticDelay>();
            last->setNext(delay_);
            last = delay_;
            delay_->init();
        } else if (it.compare("Nat") == 0){
#ifdef TRACE_LOG
            std::cout << "adding Nat filter" << std::endl;
#endif
            nat_ = std::make_shared<Nat>();
            last->setNext(nat_);
            last = nat_;
        } else if (it.compare("TokenBucketFilter") == 0){
#ifdef TRACE_LOG
            std::cout << "adding TokenBucketFilter filter" << std::endl;
#endif
            tbf_ = std::make_shared<TokenBucketFilter>();
            last->setNext(tbf_);
            last = tbf_;
            tbf_->run();
        } else {
            std::cout << "unknown filter " << it << std::endl;
        }
    }
    output_ = std::make_shared<Output>();
    last->setNext(output_);
    output_->init();
    initialized_ = true;
}

Router::~Router() {
}

int Router::newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    PacketPtr pkt = std::make_shared<Packet>(nfa);
#ifdef TRACE_LOG
    pkt->dump();
#endif
    int ret = nfq_set_verdict(qh, pkt->getNetfilterID(), NF_DROP, 0, NULL);
    next_->handlePacket(pkt);
    return ret;
}

RouterPtr Router::getInstance() {
    static RouterPtr instance_;
    static std::mutex instanceMutex_;
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if ( !instance_ ) {
        instance_ = std::make_shared<Router>();
    }
    return instance_;
}

bool Router::execute() {
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    int fd;
    int rv;
    char buf[RECV_BUF_SIZE];
    if (!initialized_){
        std::cout << "Router::execute() was called before Router::init()" << std::endl;
        return false;
    }
    h = nfq_open();
    if (!h) {
        std::cout << "error during nfq_open()" << std::endl;
        return false;
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        std::cout << "error during nfq_unbind_pf()\n" << std::endl;
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        std::cout << "error during nfq_bind_pf()" << std::endl;
        return false;
    }

    qh = nfq_create_queue(h,  0, &cb, NULL);
    if (!qh) {
        std::cout << "error during nfq_create_queue()" << std::endl;
        return false;
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        std::cout << "can't set packet_copy mode" << std::endl;
        return false;
    }

    nh = nfq_nfnlh(h);
    fd = nfnl_fd(nh);

    std::cout << "Router starting Execute loop" << std::endl;
    while ((rv = recv(fd, buf, RECV_BUF_SIZE, 0))) {
        if (rv >= 0){
            nfq_handle_packet(h, buf, rv);
        }
    }
    
    std::cout << "exiting normally with rv: " << rv << " and errno: " << errno << std::endl;
    nfq_destroy_queue(qh);
    nfq_close(h);

    return true;  
}

void Router::handlePacket(PacketPtr pkt) {}

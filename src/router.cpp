
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
}

void Router::init() {
    loss_ = std::make_shared<Loss>();
    nat_ = std::make_shared<Nat>();
    delay_ = std::make_shared<StaticDelay>();
    output_ = std::make_shared<Output>();
    setNext(loss_);
    loss_->setNext(nat_);
    nat_->setNext(delay_);
    delay_->setNext(output_);

    output_->init();
    delay_->init();
}

Router::~Router() {
}

void Router::handlePacket(PacketPtr pkt) {
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
        // Running init() with default parameters, if setDelay(), setLoss()
        // is called, init() should be called again manually
        instance_->init();
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
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        return false;
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        return false;
    }

    qh = nfq_create_queue(h,  0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        return false;
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
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

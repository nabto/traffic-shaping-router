
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
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <linux/netfilter.h>
}

#define RECV_BUF_SIZE 65536

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    RouterPtr rt = Router::getInstance();
    return rt->newPacket(qh,nfmsg,nfa,data);
}

static int cb6(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    RouterPtr rt = Router::getInstance();
    return rt->newPacket6(qh,nfmsg,nfa,data);
}

Router::Router() {
    initialized_ = false;
}

bool Router::init(std::string extIf) {
    std::cout << "using all filters" << std::endl;
    inputDrop_   = std::make_shared<InputDrop>();
    loss_    = std::make_shared<Loss>();
    nat_     = std::make_shared<Nat>();
    burst_   = std::make_shared<Burst>();
    delay_   = std::make_shared<StaticDelay>();
    dynDel_  = std::make_shared<DynamicDelay>();
    dynLoss_ = std::make_shared<DynamicLoss>();
    tbf_     = std::make_shared<TokenBucketFilter>();
    outputLibnet_ = std::make_shared<OutputLibnet>();
    inputDrop_->setNext(loss_);
    loss_->setNext(nat_);
    nat_->setNext(burst_);
    burst_->setNext(delay_);
    delay_->setNext(dynLoss_);
    dynLoss_->setNext(dynDel_);
    dynDel_->setNext(tbf_);
    tbf_->setNext(outputLibnet_);
    start_ = inputDrop_;

    input6_   = std::make_shared<Input>();
    loss6_    = std::make_shared<Loss>();
    burst6_   = std::make_shared<Burst>();
    delay6_   = std::make_shared<StaticDelay>();
    dynDel6_  = std::make_shared<DynamicDelay>();
    dynLoss6_ = std::make_shared<DynamicLoss>();
    tbf6_     = std::make_shared<TokenBucketFilter>();
    output6_  = std::make_shared<Output>();
    input6_->setNext(loss6_);
    loss6_->setNext(burst6_);
    burst6_->setNext(delay6_);
    delay6_->setNext(dynLoss6_);
    dynLoss6_->setNext(dynDel6_);
    dynDel6_->setNext(tbf6_);
    tbf6_->setNext(output6_);
    start6_ = input6_;

    extIf_ = extIf;
    initialized_ = true;
    return true;
}

bool Router::init(std::string extIf, std::vector<std::string> filters) {
    std::shared_ptr<Filter> last;
    std::shared_ptr<Filter> last6;
    extIf_ = extIf;
    for (auto it : filters){
        if (it.compare("Input") == 0){
#ifdef TRACE_LOG
            std::cout << "adding Input filter" << std::endl;
#endif
            input_ = std::make_shared<Input>();
            last = input_;
            start_ = input_;
            input6_ = std::make_shared<Input>();
            last6 = input6_;
            start6_ = input6_;
        } else if (it.compare("InputDrop") == 0){
#ifdef TRACE_LOG
            std::cout << "adding InputDrop filter" << std::endl;
#endif
            inputDrop_ = std::make_shared<InputDrop>();
            last = inputDrop_;
            start_ = inputDrop_;
            inputDrop6_ = std::make_shared<InputDrop>();
            last6 = inputDrop6_;
            start6_ = inputDrop6_;
        } else if (it.compare("Burst") == 0){
#ifdef TRACE_LOG
            std::cout << "adding burst filter" << std::endl;
#endif
            burst_ = std::make_shared<Burst>();
            last->setNext(burst_);
            last = burst_;
            burst6_ = std::make_shared<Burst>();
            last6->setNext(burst6_);
            last6 = burst6_;
        } else if (it.compare("Loss") == 0){
#ifdef TRACE_LOG
            std::cout << "adding Loss filter" << std::endl;
#endif
            loss_ = std::make_shared<Loss>();
            last->setNext(loss_);
            last = loss_;
            loss6_ = std::make_shared<Loss>();
            last6->setNext(loss6_);
            last6 = loss6_;
        } else if (it.compare("StaticDelay") == 0){
#ifdef TRACE_LOG
            std::cout << "adding StaticDelay filter" << std::endl;
#endif
            delay_ = std::make_shared<StaticDelay>();
            last->setNext(delay_);
            last = delay_;
            delay6_ = std::make_shared<StaticDelay>();
            last6->setNext(delay6_);
            last6 = delay6_;
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
            tbf6_ = std::make_shared<TokenBucketFilter>();
            last6->setNext(tbf6_);
            last6 = tbf6_;
        } else if (it.compare("DynamicDelay") == 0){
#ifdef TRACE_LOG
            std::cout << "adding DynamicDelay filter" << std::endl;
#endif
            dynDel_ = std::make_shared<DynamicDelay>();
            last->setNext(dynDel_);
            last = dynDel_;
            dynDel6_ = std::make_shared<DynamicDelay>();
            last6->setNext(dynDel6_);
            last6 = dynDel6_;
            
        } else if (it.compare("DynamicLoss") == 0){
#ifdef TRACE_LOG
            std::cout << "adding DynamicLoss filter" << std::endl;
#endif
            dynLoss_ = std::make_shared<DynamicLoss>();
            last->setNext(dynLoss_);
            last = dynLoss_;
            dynLoss6_ = std::make_shared<DynamicLoss>();
            last6->setNext(dynLoss6_);
            last6 = dynLoss6_;
            
        } else if (it.compare("Output") == 0){
#ifdef TRACE_LOG
            std::cout << "adding Output filter" << std::endl;
#endif
            output_ = std::make_shared<Output>();
            last->setNext(output_);
            last = output_;
            output6_ = std::make_shared<Output>();
            last6->setNext(output6_);
            last6 = output6_;
            
        }else if (it.compare("OutputLibnet") == 0){
#ifdef TRACE_LOG
            std::cout << "adding OutputLibnet filter" << std::endl;
#endif
            outputLibnet_ = std::make_shared<OutputLibnet>();
            last->setNext(outputLibnet_);
            last = outputLibnet_;
            output6Libnet_ = std::make_shared<Output6Libnet>();
            last6->setNext(output6Libnet_);
            last6 = output6Libnet_;
            
        }else {
            std::cout << "unknown filter " << it << std::endl;
        }
    }
    initialized_ = true;
    return true;
}

Router::~Router() {
}

int Router::newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    PacketPtr pkt = std::make_shared<Packet>(qh, nfa);
    std::string inIf;
    pkt->getInboundInterface(inIf);
    if(inIf == extIf_){
        pkt->setIngoing(true);
    } else {
        pkt->setIngoing(false);
    }
#ifdef TRACE_LOG
    std::cout << "Dumping from newPacket" << std::endl;
    pkt->dump();
#endif
    start_->handlePacket(pkt);
    return 0;
}

int Router::newPacket6(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    Packet6Ptr pkt = std::make_shared<Packet6>(qh, nfa);
    std::string inIf;
    pkt->getInboundInterface(inIf);
    if(inIf == extIf_){
        pkt->setIngoing(true);
    } else {
        pkt->setIngoing(false);
    }
#ifdef TRACE_LOG
    std::cout << "Dumping from newPacket6" << std::endl;
    pkt->dump();
#endif
    start6_->handlePacket(pkt);
    return 0;
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

    if (!initFilters(qh)) {
        std::cout << "Filter initialization failed" << std::endl;
        return false;
    }

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

bool Router::execute6() {
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    int fd;
    int rv;
    char buf[RECV_BUF_SIZE];
    if (!initialized_){
        std::cout << "Router::execute6() was called before Router::init()" << std::endl;
        return false;
    }
    h = nfq_open();
    if (!h) {
        std::cout << "error during nfq_open()" << std::endl;
        return false;
    }

    if (nfq_unbind_pf(h, AF_INET6) < 0) {
        std::cout << "error during nfq_unbind_pf()\n" << std::endl;
    }

    if (nfq_bind_pf(h, AF_INET6) < 0) {
        std::cout << "error during nfq_bind_pf()" << std::endl;
        return false;
    }

    qh = nfq_create_queue(h,  0, &cb6, NULL);
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

    if (!init6Filters(qh)) {
        std::cout << "Filter initialization failed" << std::endl;
        return false;
    }

    std::cout << "Router starting Execute6 loop" << std::endl;
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

void Router::handlePacket(std::shared_ptr<ParentPacket> pkt) {}

bool Router::initFilters(struct nfq_q_handle *qh){
    if(input_) {
        if(!input_->init()){
            std::cout << "initialization of input filter failed" << std::endl;
            return false;
        }
    }
    if(inputDrop_) {
        inputDrop_->setQH(qh);
        if(!inputDrop_->init()){
            std::cout << "initialization of inputDrop filter failed" << std::endl;
            return false;
        }
    }
    if(delay_) {
        if(!delay_->init()){
            std::cout << "initialization of delay filter failed" << std::endl;
            return false;
        }
    }
    if(dynDel_) {
        if(!dynDel_->init()){
            std::cout << "initialization of Dynamic delay filter failed" << std::endl;
            return false;
        }
    }
    if(dynLoss_) {
        if(!dynLoss_->init()){
            std::cout << "initialization of Dynamic loss filter failed" << std::endl;
            return false;
        }
    }
    if(loss_) {
        if(!loss_->init()){
            std::cout << "initialization of loss filter failed" << std::endl;
            return false;
        }
    }
    if(nat_) {
        if(!nat_->init()){
            std::cout << "initialization of nat filter failed" << std::endl;
            return false;
        }
    }
    if(burst_) {
        if(!burst_->init()){
            std::cout << "initialization of burst filter failed" << std::endl;
            return false;
        }
    }
    if(tbf_) {
        if(!tbf_->init()){
            std::cout << "initialization of tbf filter failed" << std::endl;
            return false;
        }
    }
    if(output_) {
        output_->setQH(qh);
        if(!output_->init()){
            std::cout << "initialization of output filter failed" << std::endl;
            return false;
        }
    }
    if(outputLibnet_) {
        if(!outputLibnet_->init()){
            std::cout << "initialization of outputLibnet filter failed" << std::endl;
            return false;
        }
    }
    return true;
}

bool Router::init6Filters(struct nfq_q_handle *qh){
    if(input6_) {
        if(!input6_->init()){
            std::cout << "initialization of input 6 filter failed" << std::endl;
            return false;
        }
    }
    if(inputDrop6_) {
        inputDrop6_->setQH(qh);
        if(!inputDrop6_->init()){
            std::cout << "initialization of inputDrop 6 filter failed" << std::endl;
            return false;
        }
    }
    if(delay6_) {
        if(!delay6_->init()){
            std::cout << "initialization of delay 6 filter failed" << std::endl;
            return false;
        }
    }
    if(dynDel6_) {
        if(!dynDel6_->init()){
            std::cout << "initialization of Dynamic delay 6 filter failed" << std::endl;
            return false;
        }
    }
    if(dynLoss6_) {
        if(!dynLoss6_->init()){
            std::cout << "initialization of Dynamic Loss 6 filter failed" << std::endl;
            return false;
        }
    }
    if(loss6_) {
        if(!loss6_->init()){
            std::cout << "initialization of loss 6 filter failed" << std::endl;
            return false;
        }
    }
    if(burst6_) {
        if(!burst6_->init()){
            std::cout << "initialization of burst 6 filter failed" << std::endl;
            return false;
        }
    }
    if(tbf6_) {
        if(!tbf6_->init()){
            std::cout << "initialization of tbf 6 filter failed" << std::endl;
            return false;
        }
    }
    if(output6_) {
        output_->setQH(qh);
        if(!output6_->init()){
            std::cout << "initialization of output 6 filter failed" << std::endl;
            return false;
        }
    }
    if(output6Libnet_) {
        if(!output6Libnet_->init()){
            std::cout << "initialization of outputLibnet 6 filter failed" << std::endl;
            return false;
        }
    }
    return true;
}

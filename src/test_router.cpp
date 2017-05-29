
#include <stddef.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <mutex>
#include <stdint.h>
#include "router.hpp"
#include "structures.h"
#include <functional>
#include <sys/time.h>

extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
}

std::vector<struct timeval> time_;

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
	RouterPtr rt = Router::getInstance();
	return rt->newPacket(qh,nfmsg,nfa,data);
}

Router::Router(){
}

void Router::init(){
    id_ = 0;
    auto self = shared_from_this();
    nat_ = std::make_shared<Nat>();
    delay_ = std::make_shared<StaticDelay>(100);
    loss_ = std::make_shared<Loss>(0.1);
    setNext(nat_);
    nat_->setNext(loss_);
    loss_->setNext(delay_);
    delay_->setNext(self);

}

Router::~Router(){
}

void Router::handlePacket(Packet pkt){
    std::cout << "Router sending packet" << std::endl;
    struct timeval stop;
    struct timeval start = time_[0];
    gettimeofday(&stop, NULL);
    std::cout << "stop time was: " << stop.tv_usec << "us. Packet delay was: " << (stop.tv_sec-start.tv_sec)*1000000+ stop.tv_usec-start.tv_usec <<  "us"<< std::endl;
    time_.erase(time_.begin());
//    pkt.send();
}

int Router::newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    Packet pkt;
    std::cout << "starting timer" << std::endl;
    struct timeval start;
    gettimeofday(&start, NULL);
    time_.push_back(start);
    std::cout << "start time was: " << start.tv_usec << "us" << std::endl;
//    ioService_.post(boost::bind(f, pkt));
    next_->handlePacket(pkt);
    return 0;
}

RouterPtr Router::getInstance()
{
    static RouterPtr instance_;
    static std::mutex mutex2_;
    std::lock_guard<std::mutex> lock(mutex2_);
    if ( !instance_ ) {
        instance_ = std::make_shared<Router>();
    }
    instance_->init();
    return instance_;
}

bool Router::execute()
{
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[4096];
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
	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {	
        nfq_handle_packet(h, buf, rv);
	}

	nfq_destroy_queue(qh);

	nfq_close(h);

	return true;  
}

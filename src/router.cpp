
#include <stddef.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <mutex>
#include <stdint.h>
#include "router.hpp"
#include "structures.h"
#include <functional>


extern "C" {
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
	RouterPtr rt = Router::getInstance();
	return rt->handlePacket(qh,nfmsg,nfa,data);
}

// void processPacketAsync(Packet pkt, std::function<void (int) > cb){
//     pkt.processAsync(cb);
// }

Router::Router(){
    id_ = 0;
    //boost::asio::io_service::work work(ioService_);
    work_.reset(new boost::asio::io_service::work(ioService_));
    threadpool_.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService_)
        );
    threadpool_.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService_)
        );

    
}

Router::~Router(){
    ioService_.stop();
    threadpool_.join_all();
}

void Router::packetCallback(Packet* pkt){
    std::lock_guard<std::mutex> lock(mutex_);
    if(pkt->getVerdict() == READY_TO_SEND){
        std::cout << "verdict was READY_TO_SEND" << std::endl;
        pkt->send();
    } else if(pkt->getVerdict() == WAITING){
        std::cout << "verdict was WAITING" << std::endl;
    } else if(pkt->getVerdict() == DROPPED){
        std::cout << "verdict was DROPPED" << std::endl;
    } else {
        std::cout << "verdict undefined" << std::endl;
    }
    int i = pkt->getId();
    std::cout << "packets_.size() before: " << packets_.size() << std::endl;
    auto it = find_if(packets_.begin(), packets_.end(), [i](Packet& obj) {return obj.getId() == i;});
    //if(it != packets_.end()){
    packets_.erase(it);
    //}
    std::cout << "packets_.size() after: " << packets_.size() << std::endl;
}

int Router::handlePacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
    auto self = shared_from_this();
    std::function< void (Packet&)> f = [self] (Packet& pkt) {
        std::cout << "Starting Async handling" << std::endl;
        pkt.processAsync();
        self->packetCallback(&pkt);
    };
    Packet pkt(nfa, id_);
    id_++;
    std::lock_guard<std::mutex> lock(self->mutex_);
    pkt.setIndex(packets_.size());
    packets_.push_back(pkt);
//    std::cout << "new packet received from " << unsigned(pkt.getSourceIP()) << " to " << unsigned(pkt.getDestinationIP()) << std::endl;
    int ret = nfq_set_verdict(qh, pkt.getNetfilterID(), NF_DROP, 0, NULL);
    ioService_.post(boost::bind(f, boost::ref(packets_.back())));
    return ret;
}

RouterPtr Router::getInstance()
{
    static RouterPtr instance_;
    static std::mutex mutex2_;
    std::lock_guard<std::mutex> lock(mutex2_);
    if ( !instance_ ) {
        instance_ = std::make_shared<Router>();
    }
    
    return instance_;
}

bool Router::init()
{
    return true;
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

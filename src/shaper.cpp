#include "shaper.hpp"
#include "packet.hpp"
#include "structures.h"
#include <unistd.h>

Shaper::Shaper(){
    boost::asio::io_service::work work(ioService_);
    work_.reset(new boost::asio::io_service::work(ioService_));
    threadpool_.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService_)
        );
    threadpool_.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService_)
        );
}

Shaper::~Shaper(){
    ioService_.stop();
    //threadpool_.join_all();
}

void Shaper::handlePacket(Packet pkt){
    std::cout << "shaping packet" << std::endl;
    // if (rand() % 100 < 10){
    //     pkt.setVerdict(DROPPED);
    //     return;
    // }
    next_->handlePacket(pkt);
    return;
    queue_.push(pkt);
    
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(ioService_, boost::posix_time::milliseconds(100));
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t] (void) {
        std::cout << "lambda function triggered" << std::endl;
        self->queueTimeEvent();
    };
    t->async_wait(boost::bind(f));
    // usleep(10000);
    // queueTimeEvent();
    std::cout << "shaper returning async from pkt:" << std::endl;
    pkt.dump();
}

void Shaper::queueTimeEvent(){
    std::cout << "queueTimeEvent triggered on packet:" << std::endl;
    Packet pkt = queue_.front();
    pkt.setVerdict(ACCEPTED);
    pkt.dump(); 
    next_->handlePacket(pkt);
    // queue_.front().setVerdict(ACCEPTED);
    // next_->handlePacket(queue_.front());
    queue_.pop();
    
}

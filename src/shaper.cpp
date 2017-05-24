#include "shaper.hpp"
#include "packet.hpp"
#include "structures.h"
#include <unistd.h>

Shaper::Shaper(int delay, float loss){
    delay_ = delay;
    loss_ = loss;
    tp_ = TpService::getInstance();
    ioService_ = tp_->getIoService();
}

Shaper::~Shaper(){
}

void Shaper::handlePacket(Packet pkt){
    std::cout << "shaping packet" << std::endl;
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (r < loss_){
        pkt.setVerdict(DROPPED);
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(pkt);
    }
    /*usleep(10000);
    queueTimeEvent();
    //next_->handlePacket(pkt);
    return;*/
    
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, boost::posix_time::milliseconds(delay_));
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t] (void) {
        std::cout << "lambda function triggered" << std::endl;
        self->queueTimeEvent();
    };
    t->async_wait(boost::bind(f));
    std::cout << "shaper returning async from pkt:" << std::endl;
    pkt.dump();
}

void Shaper::queueTimeEvent(){
    std::cout << "queueTimeEvent triggered on packet:" << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    Packet pkt = queue_.front();
    pkt.setVerdict(ACCEPTED);
    pkt.dump(); 
    next_->handlePacket(pkt);
    // queue_.front().setVerdict(ACCEPTED);
    // next_->handlePacket(queue_.front());
    queue_.pop();
    
}

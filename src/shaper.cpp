#include "shaper.hpp"
#include "packet.hpp"
#include "structures.h"
#include <unistd.h>

StaticDelay::StaticDelay(int delay){
    delay_ = boost::posix_time::milliseconds(delay);
    tp_ = TpService::getInstance();
    ioService_ = tp_->getIoService();
}

StaticDelay::~StaticDelay(){
}

void StaticDelay::handlePacket(Packet pkt){
#ifdef TRACE_LOG
    std::cout << std::endl << "Delay Filter" << std::endl;
#endif
    boost::posix_time::time_duration delay;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(pkt);
#ifdef TRACE_LOG
        std::cout << "pushed to queue, Size: " << queue_.size() << std::endl;
        pkt.dump();
#endif
        if(queue_.size() > 1){
            return;
        }
        // The new Packet is the only one inqueued
        boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
        delay = delay_ - (now - queue_.front().getTimeStamp());
        if (delay < boost::posix_time::microseconds(0)){
            delay = boost::posix_time::microseconds(0);
        }
    }
    /*usleep(10000);
    queueTimeEvent();
    //next_->handlePacket(pkt);
    return;*/
#ifdef TRACE_LOG
    std::cout << "handlepacket making delay of: " << boost::posix_time::to_iso_string(delay) << std::endl;
    std::cout << "time is now: " << boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) << std::endl;
#endif
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, delay);
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t] (void) {
        self->queueTimeEvent();
    };
    t->async_wait(boost::bind(f));
}

void StaticDelay::queueTimeEvent(){
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef TRACE_LOG
    std::cout << "queueTimeEvent triggered at: " << boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) << std::endl;
//    std::cout << "queueTimeEvent using front packet ID: " << queue_.front().getNetfilterID() << std::endl;
    if(queue_.size() == 0){
        std::cout << "tried to access empty queue" << std::endl;
    }
//    pkt.dump();
#endif
    Packet pkt = queue_.front();
    pkt.resetTimeStamp();
    next_->handlePacket(pkt);
    queue_.pop();
    if(queue_.size() > 0){
        boost::posix_time::time_duration delay;
        boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
        delay = delay_ - (now - queue_.front().getTimeStamp());
        if (delay < boost::posix_time::milliseconds(0)){
            delay = boost::posix_time::milliseconds(0);
        }
    
#ifdef TRACE_LOG
        std::cout << "queueTimeEvent making delay of: " << boost::posix_time::to_iso_string(delay) << std::endl;
#endif
        std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, delay);
        auto self = shared_from_this();
        std::function< void (void)> f = [self, t] (void) {
            self->queueTimeEvent();
        };
        t->async_wait(boost::bind(f));
    }
//    std::cout << "queueTimeEvent popped front now has packet ID: " << queue_.front().getNetfilterID() << std::endl;

}

Loss::Loss(float loss){
    loss_ = loss;
}

Loss::~Loss(){
}

void Loss::handlePacket(Packet pkt){
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (r < loss_){
        return;
    }
    next_->handlePacket(pkt);
}

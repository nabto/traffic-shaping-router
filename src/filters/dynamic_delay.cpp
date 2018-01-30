#include "dynamic_delay.hpp"
#include "packet.hpp"
#include <unistd.h>


DynamicDelay::DynamicDelay(): queue_(*(TpService::getInstance()->getIoService())), delayQ_(*(TpService::getInstance()->getIoService())) {
    delay_ = boost::posix_time::milliseconds(0);
    int_ = boost::posix_time::milliseconds(1000);
    ioService_ = TpService::getInstance()->getIoService();
}

std::vector<int> DynamicDelay::setDelays(std::vector<int> delays, int resolution){
    std::vector<int> tmpQ;
    tmpQ.push_back(delays[0]);
    delays.push_back(delays[0]);
    for (size_t i = 1; i<delays.size(); i++){
        int first = tmpQ.back();
        for (int a = 1; a<=resolution; a++){
            tmpQ.push_back(((float)delays[i]-delays[i-1])/resolution*a+first);
        }
            
    }
    tmpQ.pop_back();
    for ( auto i : tmpQ){
        delayQ_.push(i);
    }
    return tmpQ;
}

bool DynamicDelay::init() {
    queue_.asyncPop(std::bind(&DynamicDelay::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    delayQ_.asyncPop(std::bind(&DynamicDelay::delayPopHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    return true;
}

DynamicDelay::~DynamicDelay() {
}

void DynamicDelay::handlePacket(std::shared_ptr<ParentPacket> pkt) {
    queue_.push(pkt);
}

void DynamicDelay::delayPopHandler(const std::error_code& ec, const int d) {
    delayQ_.push(d);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        delay_ =  boost::posix_time::milliseconds(d);
    }    
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, int_);
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t, d] (void) {
        self->delayQ_.asyncPop(std::bind(&DynamicDelay::delayPopHandler, self, std::placeholders::_1, std::placeholders::_2));
    };
    t->async_wait(boost::bind(f));
}
void DynamicDelay::popHandler(const std::error_code& ec, const std::shared_ptr<ParentPacket> pkt) {
    boost::posix_time::time_duration delay;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    if(ec == queue_error_code::stopped){
        std::cout << "DynamicDelay popHandler got queue stopped" << std::endl;
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        delay = delay_ - (now - pkt->getTimeStamp());
    }
    if (delay < boost::posix_time::milliseconds(0)){
        delay = boost::posix_time::milliseconds(0);
    }
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, delay);
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t, pkt] (void) {
        // resetting time stamp in case of more delay filters
        pkt->resetTimeStamp();
        self->next_->handlePacket(pkt);
        self->queue_.asyncPop(std::bind(&DynamicDelay::popHandler, self, std::placeholders::_1, std::placeholders::_2));
    };
    t->async_wait(boost::bind(f));
}

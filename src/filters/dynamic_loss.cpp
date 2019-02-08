#include "dynamic_loss.hpp"
#include "packet.hpp"

DynamicLoss::DynamicLoss() {
    ioService_ = TpService::getInstance()->getIoService();
    std::vector<int> tmp(1);
    tmp[0] = 1;
    times_ = tmp;
    std::vector<float> tmp2(1);
    tmp2[0] = 0;
    losses_ = tmp2;
    srand (static_cast <unsigned> (time(0)));
}

DynamicLoss::~DynamicLoss() {

}

bool DynamicLoss::init() {
    currentLoss_ = losses_[0];
    currentIndex_ = 0;
    
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, boost::posix_time::milliseconds(times_[currentIndex_]*1000));
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t] (void) {
        self->handleTimeout();
    };
    t->async_wait(boost::bind(f));
    return true;
}

void DynamicLoss::handleTimeout() {
    currentIndex_++;
    if ( currentIndex_ >= losses_.size()) {
        currentIndex_ = 0;
    }
    currentLoss_ = losses_[currentIndex_];
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, boost::posix_time::milliseconds(times_[currentIndex_]*1000));
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t] (void) {
        self->handleTimeout();
    };
    t->async_wait(boost::bind(f));
}

void DynamicLoss::handlePacket(std::shared_ptr<ParentPacket> pkt) {
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (r < currentLoss_){
        pkt->setVerdict(false);
        return;
    }
    next_->handlePacket(pkt);
}

bool DynamicLoss::setLosses(std::vector<int> times, std::vector<float> losses) {
    if (times.size() != losses.size()) {
        return false;
    } else {
        times_ = times;
        losses_ = losses;
        return true;
    }
}



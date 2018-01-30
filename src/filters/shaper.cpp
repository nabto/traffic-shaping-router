#include "shaper.hpp"
#include "packet.hpp"
#include <unistd.h>

/* ========= STATIC DELAY FILTER ==========*/

StaticDelay::StaticDelay(): queue_(*(TpService::getInstance()->getIoService())) {
    delay_ = boost::posix_time::milliseconds(0);
    ioService_ = TpService::getInstance()->getIoService();
}
bool StaticDelay::init() {
    queue_.asyncPop(std::bind(&StaticDelay::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    return true;
}

StaticDelay::~StaticDelay() {
}

void StaticDelay::handlePacket(std::shared_ptr<ParentPacket> pkt) {
    queue_.push(pkt);
}

void StaticDelay::popHandler(const std::error_code& ec, const std::shared_ptr<ParentPacket> pkt) {
    boost::posix_time::time_duration delay;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    if(ec == queue_error_code::stopped){
        std::cout << "StaticDelay popHandler got queue stopped" << std::endl;
        return;
    }
    delay = delay_ - (now - pkt->getTimeStamp());
    if (delay < boost::posix_time::milliseconds(0)){
        delay = boost::posix_time::milliseconds(0);
    }
    std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, delay);
    auto self = shared_from_this();
    std::function< void (void)> f = [self, t, pkt] (void) {
        // resetting time stamp in case of more delay filters
        pkt->resetTimeStamp();
        self->next_->handlePacket(pkt);
        self->queue_.asyncPop(std::bind(&StaticDelay::popHandler, self, std::placeholders::_1, std::placeholders::_2));
    };
    t->async_wait(boost::bind(f));
}
   

/* ========== LOSS FILTER =========== */

void Loss::handlePacket(std::shared_ptr<ParentPacket> pkt) {
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (r < loss_){
        pkt->setVerdict(false);
        return;
    }
    next_->handlePacket(pkt);
}

#include "shaper.hpp"
#include "packet.hpp"
#include <unistd.h>

/* ========= STATIC DELAY FILTER ==========*/

StaticDelay::StaticDelay(int delay): queue_(*(TpService::getInstance()->getIoService())) {
    delay_ = boost::posix_time::milliseconds(delay);
    tp_ = TpService::getInstance();
    ioService_ = tp_->getIoService();
}
void StaticDelay::init() {
    queue_.asyncPop(std::bind(&StaticDelay::popHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

StaticDelay::~StaticDelay() {
}

void StaticDelay::handlePacket(PacketPtr pkt) {
    {
        //std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(pkt);
        /*if(queue_.size() > 1){
            // If this is not the only packet in queue,
            // queueTimeEvent will schedule new events
            return;
            }*/
    }
    //scheduleEvent();
}
/*
void StaticDelay::queueTimeEvent() {
    PacketPtr pkt;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() == 0){
            std::cout << "BAAAD" << std::endl;
            return;
        }
        pkt = queue_.front();
        queue_.pop();
    }
    // resetting time stamp in case of more delay filters
    pkt->resetTimeStamp();
    next_->handlePacket(pkt);
    if(queue_.size() < 1){
        return;
    }
    
    scheduleEvent();
}

void StaticDelay::scheduleEvent() {
    boost::posix_time::time_duration delay;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    {
        std::lock_guard<std::mutex> lock(mutex_);

        delay = delay_ - (now - queue_.front()->getTimeStamp());
        if (delay < boost::posix_time::milliseconds(0)){
            delay = boost::posix_time::milliseconds(0);
        }
        std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, delay);
        auto self = shared_from_this();
        std::function< void (void)> f = [self, t] (void) {
            self->queueTimeEvent();
            // call popHandler async from here
        };
        t->async_wait(boost::bind(f));
    }
}
*/
void StaticDelay::popHandler(const boost::system::error_code& ec, const PacketPtr pkt) {
    boost::posix_time::time_duration delay;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
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

Loss::Loss(float loss) {
    loss_ = loss;
}

Loss::~Loss() {
}

void Loss::handlePacket(PacketPtr pkt) {
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (r < loss_){
        return;
    }
    next_->handlePacket(pkt);
}

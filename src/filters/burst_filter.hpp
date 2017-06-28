#pragma once
#include <filter.hpp>
#include <packet.hpp>

class Burst : public Filter, public std::enable_shared_from_this<Burst>
{
 public:
    Burst() {
        ioService_ = TpService::getInstance()->getIoService();
        burstDur_ = boost::posix_time::milliseconds(0);
        sleepDur_ = boost::posix_time::milliseconds(0);
        sleeping_ = false;
    }
    ~Burst() {}
    void run(){
        std::shared_ptr<boost::asio::deadline_timer> t;
        if(sleeping_) {
            t = std::make_shared<boost::asio::deadline_timer>(*ioService_, burstDur_);
            sleeping_ = false;
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            for(auto it : queue_) {
                next_->handlePacket(it);
            }
            queue_.clear();
            t = std::make_shared<boost::asio::deadline_timer>(*ioService_, sleepDur_);
            sleeping_ = true;
        }
        auto self = shared_from_this();
        std::function< void (void)> f = [self, t] (void) {
            self->run();
        };
        t->async_wait(boost::bind(f));
        
    }
    void handlePacket(PacketPtr pkt){
        if(sleeping_){
            next_->handlePacket(pkt);
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(pkt);
        }
        
    }

    void setBurstDuration(int dur){burstDur_ = boost::posix_time::milliseconds(dur);}
    void setSleepDuration(int sleep){sleepDur_ = boost::posix_time::milliseconds(sleep);}
    bool isSleeping(){return sleeping_;}
    void startSleeping(){sleeping_ = true;}
    void stopSleeping(){sleeping_ = false;}
 private:
    std::vector<PacketPtr> queue_;
    std::mutex mutex_;
    boost::posix_time::time_duration burstDur_;
    boost::posix_time::time_duration sleepDur_;
    boost::asio::io_service* ioService_;
    bool sleeping_;
};

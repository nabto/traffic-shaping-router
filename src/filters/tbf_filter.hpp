#pragma once
#include <filter.hpp>
#include <packet.hpp>
#include <sys/time.h>

//#define TRACE_LOG

class TokenBucketFilter : public Filter, public std::enable_shared_from_this<TokenBucketFilter>
{
 public:
    TokenBucketFilter() {
        srand (static_cast <unsigned> (time(0)));
        ioService_ = TpService::getInstance()->getIoService();
        timerStart_ = getTimeStamp();
        tokenInterval_ = boost::posix_time::milliseconds(4);
        maxTokens_ = 8000;
        maxPackets_ = 30;
        dataRate_ = 20480;
        redDrop_ = 0;
        redStart_ = maxPackets_;
    }
    ~TokenBucketFilter() {}

    void run() {
        if(tokens_ < maxTokens_){
            uint64_t diff = getTimeStamp()-timerStart_;
            timerStart_ = getTimeStamp();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tokens_ += dataRate_*diff/1000/8;
                if(tokens_ > maxTokens_){
                    tokens_ = maxTokens_;
                }
            }
            trySend();
        }
        std::shared_ptr<boost::asio::deadline_timer> t = std::make_shared<boost::asio::deadline_timer>(*ioService_, tokenInterval_);
        auto self = shared_from_this();
        std::function< void (void)> f = [self, t] (void) {
            self->run();
        };
        t->async_wait(boost::bind(f));
        
    }
    
    void handlePacket(PacketPtr pkt) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(queue_.size() <  maxPackets_){
                float r = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
                if(queue_.size() < redStart_){
                    queue_.push(pkt);
                } else if (r > redDrop_) {
                    queue_.push(pkt);
                }
            }
#ifdef TRACE_LOG
            else {
                std::cout << "Queue full packet dropped" << std::endl;
            }
#endif
        }
        trySend();
    }

    void setMaxTokens(uint32_t t) {maxTokens_ = t;}
    void setMaxPackets(uint32_t p) {maxPackets_ = p;}
    void setRateLimit(uint32_t rate) {dataRate_ = rate;}
    void setRedStart(uint32_t start) {redStart_ = start;}
    void setRedDrop(float drop) {redDrop_ = drop;}
 private:
    std::mutex mutex_;
    std::queue<PacketPtr> queue_;
    uint32_t tokens_;
    uint32_t maxTokens_;
    uint32_t maxPackets_;
    uint32_t dataRate_; // in kbit pr sec
    uint64_t timerStart_;
    uint32_t redStart_;
    float redDrop_;
    boost::asio::io_service* ioService_;
    boost::posix_time::time_duration tokenInterval_;
    
    void trySend(){
        std::lock_guard<std::mutex> lock(mutex_);
        while(queue_.size() > 0 && queue_.front()->getPacketLen() <= tokens_ ){
            tokens_ -= queue_.front()->getPacketLen();
            next_->handlePacket(queue_.front());
            queue_.pop();
        }
    }
    uint64_t getTimeStamp() {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
    }
};

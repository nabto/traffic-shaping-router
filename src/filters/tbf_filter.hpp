#pragma once
#include <filter.hpp>
#include <packet.hpp>
#include <sys/time.h>

class TokenBucket : public Filter, public std::enable_shared_from_this<TokenBucket>
{
 public:
    TokenBucket() : tokens_(0), maxTokens_(8000), maxPackets_(30), dataRate_(20480), redDrop_(0), timerStart_(getTimeStamp()), tokenInterval_(boost::posix_time::milliseconds(4)) {
        srand (static_cast <unsigned> (time(0)));
        ioService_ = TpService::getInstance()->getIoService();
        redStart_ = maxPackets_;
    }
    ~TokenBucket() {}

    void run() {
        uint64_t diff = getTimeStamp()-timerStart_;
        timerStart_ = getTimeStamp();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(tokens_ < maxTokens_){
                {
                    tokens_ += dataRate_*diff/1000/8;
                    if(tokens_ > maxTokens_){
                        tokens_ = maxTokens_;
                    }
                }
            }
        }
        trySend();
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
    void setDataRate(uint32_t rate) {dataRate_ = rate;}
    void setRedStart(uint32_t start) {redStart_ = start;}
    void setRedDrop(float drop) {redDrop_ = drop;}

 private:
    std::mutex mutex_;
    std::queue<PacketPtr> queue_;
    uint32_t tokens_;
    uint32_t maxTokens_;
    uint32_t maxPackets_;
    uint32_t dataRate_; // in kbit pr sec
    uint32_t redStart_;
    float redDrop_;

    uint64_t timerStart_;
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


// =========== FILTER IMPLEMENTATION =============== //
class TokenBucketFilter : public Filter, public std::enable_shared_from_this<TokenBucketFilter>
{
 public:
    TokenBucketFilter() {
        inTb_ = std::make_shared<TokenBucket>();
        outTb_ = std::make_shared<TokenBucket>();
    }
    ~TokenBucketFilter() {}

    void run() {
        inTb_->setNext(next_);
        outTb_->setNext(next_);
        inTb_->run();
        outTb_->run();
    }
    
    void handlePacket(PacketPtr pkt) {
        if(pkt->isIngoing()) {
            inTb_->handlePacket(pkt);
        } else {
            outTb_->handlePacket(pkt);
        }
    }

    void setInMaxTokens(uint32_t t) {inTb_->setMaxTokens(t);}
    void setInMaxPackets(uint32_t p) {inTb_->setMaxPackets(p);}
    void setInRateLimit(uint32_t rate) {inTb_->setDataRate(rate);}
    void setInRedStart(uint32_t start) {inTb_->setRedStart(start);}
    void setInRedDrop(float drop) {inTb_->setRedDrop(drop);}
    void setOutMaxTokens(uint32_t t) {outTb_->setMaxTokens(t);}
    void setOutMaxPackets(uint32_t p) {outTb_->setMaxPackets(p);}
    void setOutRateLimit(uint32_t rate) {outTb_->setDataRate(rate);}
    void setOutRedStart(uint32_t start) {outTb_->setRedStart(start);}
    void setOutRedDrop(float drop) {outTb_->setRedDrop(drop);}

 private:
    std::shared_ptr<TokenBucket> inTb_;
    std::shared_ptr<TokenBucket> outTb_;
};


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

        inTokens_ = 0;
        inMaxTokens_ = 8000;
        inMaxPackets_ = 30;
        inDataRate_ = 20480;
        inRedDrop_ = 0;
        inRedStart_ = inMaxPackets_;

        outTokens_ = 0;
        outMaxTokens_ = 8000;
        outMaxPackets_ = 30;
        outDataRate_ = 20480;
        outRedDrop_ = 0;
        outRedStart_ = outMaxPackets_;
    }
    ~TokenBucketFilter() {}

    void run() {
        uint64_t diff = getTimeStamp()-timerStart_;
        timerStart_ = getTimeStamp();
        if(inTokens_ < inMaxTokens_){
            {
                std::lock_guard<std::mutex> lock(inMutex_);
                inTokens_ += inDataRate_*diff/1000/8;
                if(inTokens_ > inMaxTokens_){
                    inTokens_ = inMaxTokens_;
                }
            }
            trySend();
        }
        if(outTokens_ < outMaxTokens_){
            {
                std::lock_guard<std::mutex> lock(outMutex_);
                outTokens_ += outDataRate_*diff/1000/8;
                if(outTokens_ > outMaxTokens_){
                    outTokens_ = outMaxTokens_;
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
        if(pkt->isIngoing()) {
            std::lock_guard<std::mutex> lock(inMutex_);
            if(inQueue_.size() <  inMaxPackets_){
                float r = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
                if(inQueue_.size() < inRedStart_){
                    inQueue_.push(pkt);
                } else if (r > inRedDrop_) {
                    inQueue_.push(pkt);
                }
            }
#ifdef TRACE_LOG
            else {
                std::cout << "inQueue full packet dropped" << std::endl;
            }
#endif
        } else {
            std::lock_guard<std::mutex> lock(outMutex_);
            if(outQueue_.size() <  outMaxPackets_){
                float r = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
                if(outQueue_.size() < outRedStart_){
                    outQueue_.push(pkt);
                } else if (r > outRedDrop_) {
                    outQueue_.push(pkt);
                }
            }
#ifdef TRACE_LOG
            else {
                std::cout << "outQueue full packet dropped" << std::endl;
            }
#endif

        }
        trySend();
    }

    void setInMaxTokens(uint32_t t) {inMaxTokens_ = t;}
    void setInMaxPackets(uint32_t p) {inMaxPackets_ = p;}
    void setInRateLimit(uint32_t rate) {inDataRate_ = rate;}
    void setInRedStart(uint32_t start) {inRedStart_ = start;}
    void setInRedDrop(float drop) {inRedDrop_ = drop;}
    void setOutMaxTokens(uint32_t t) {outMaxTokens_ = t;}
    void setOutMaxPackets(uint32_t p) {outMaxPackets_ = p;}
    void setOutRateLimit(uint32_t rate) {outDataRate_ = rate;}
    void setOutRedStart(uint32_t start) {outRedStart_ = start;}
    void setOutRedDrop(float drop) {outRedDrop_ = drop;}

    void dump(){
        std::cout << "inTokens: " << inTokens_ << " inMaxTokens: " << inMaxTokens_ << " inMaxPackets: " << inMaxPackets_ << " inDataRate: " << inDataRate_ << " inRedStart: " << inRedStart_ << " inRedDrop: " << inRedDrop_ << std::endl;
        std::cout << "outTokens: " << outTokens_ << " outMaxTokens: " << outMaxTokens_ << " outMaxPackets: " << outMaxPackets_ << " outDataRate: " << outDataRate_ << " outRedStart: " << outRedStart_ << " outRedDrop: " << outRedDrop_ << std::endl;
    }
 private:
    std::mutex inMutex_;
    std::queue<PacketPtr> inQueue_;
    uint32_t inTokens_;
    uint32_t inMaxTokens_;
    uint32_t inMaxPackets_;
    uint32_t inDataRate_; // in kbit pr sec
    uint32_t inRedStart_;
    float inRedDrop_;

    std::mutex outMutex_;
    std::queue<PacketPtr> outQueue_;
    uint32_t outTokens_;
    uint32_t outMaxTokens_;
    uint32_t outMaxPackets_;
    uint32_t outDataRate_; // in kbit pr sec
    uint32_t outRedStart_;
    float outRedDrop_;

    uint64_t timerStart_;
    boost::asio::io_service* ioService_;
    boost::posix_time::time_duration tokenInterval_;
    
    void trySend(){
        {
            std::lock_guard<std::mutex> lock(inMutex_);
            while(inQueue_.size() > 0 && inQueue_.front()->getPacketLen() <= inTokens_ ){
                inTokens_ -= inQueue_.front()->getPacketLen();
                next_->handlePacket(inQueue_.front());
                inQueue_.pop();
            }
        }
        {
            std::lock_guard<std::mutex> lock(outMutex_);
            while(outQueue_.size() > 0 && outQueue_.front()->getPacketLen() <= outTokens_ ){
                outTokens_ -= outQueue_.front()->getPacketLen();
                next_->handlePacket(outQueue_.front());
                outQueue_.pop();
            }
        }
    }
    uint64_t getTimeStamp() {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
    }
};

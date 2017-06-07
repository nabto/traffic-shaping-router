#ifndef FILTER_HPP // one-time include
#define FILTER_HPP

#include <memory>
#include "packet.hpp"

class Filter : public std::enable_shared_from_this<Filter>{
 public:
    virtual void setNext(std::shared_ptr<Filter> next){next_ = next;}
    virtual void handlePacket(PacketPtr pkt) = 0;
 protected:
//    Filter* next_;
    std::shared_ptr<Filter> next_;
 public:
    virtual void queueTimeEvent(){}
};

#endif // FILTER_HPP

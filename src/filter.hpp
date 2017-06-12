#ifndef FILTER_HPP // one-time include
#define FILTER_HPP

#include <memory>
#include "packet.hpp"

class Filter{
 public:
    virtual void setNext(std::shared_ptr<Filter> next){next_ = next;}
    virtual void handlePacket(PacketPtr pkt) = 0;
 protected:
    std::shared_ptr<Filter> next_;
};

#endif // FILTER_HPP

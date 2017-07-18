#ifndef FILTER_HPP // one-time include
#define FILTER_HPP

#include <memory>
#include "packet.hpp"

/* Filter class which all filters must inherit */
class Filter{
 public:
    virtual void setNext(std::shared_ptr<Filter> next){next_ = next;}
    virtual void handlePacket(PacketPtr pkt) = 0;
    virtual bool init(){return true;}
 protected:
    std::shared_ptr<Filter> next_;
};

#endif // FILTER_HPP

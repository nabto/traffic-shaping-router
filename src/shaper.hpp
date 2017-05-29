#ifndef SHAPER_HPP
#define SHAPER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "filter.hpp"
#include "packet.hpp"
#include "tpService.hpp"
#include <queue>
#include <mutex>

class StaticDelay : public Filter
{
 public:
    StaticDelay(int delay);
    ~StaticDelay();
    void handlePacket(Packet pkt);
    void queueTimeEvent();

 private:
    boost::posix_time::time_duration delay_;
    std::shared_ptr<TpService>  tp_;
    std::mutex mutex_;
    std::queue<Packet> queue_;
    boost::asio::io_service* ioService_;
};

class Loss : public Filter
{
 public:
    Loss(float loss);
    ~Loss();
    void handlePacket(Packet pkt);

 private:
    float loss_;
};

#endif // SHAPER_HPP

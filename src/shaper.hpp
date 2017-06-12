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

class StaticDelay : public Filter, public std::enable_shared_from_this<StaticDelay>
{
 public:
    StaticDelay(int delay);
    ~StaticDelay();
    void handlePacket(PacketPtr pkt);
    void queueTimeEvent();

 private:
    void scheduleEvent();
    boost::posix_time::time_duration delay_;
    std::shared_ptr<TpService>  tp_;
    std::mutex mutex_;
    std::queue<PacketPtr> queue_;
    boost::asio::io_service* ioService_;
};

class Loss : public Filter, public std::enable_shared_from_this<Loss>
{
 public:
    Loss(float loss);
    ~Loss();
    void handlePacket(PacketPtr pkt);

 private:
    float loss_;
};

#endif // SHAPER_HPP

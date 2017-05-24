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

class Shaper : public Filter
{
 public:
    Shaper(int delay, float loss);
    ~Shaper();
    void handlePacket(Packet pkt);
    void queueTimeEvent();

 private:
    int delay_;
    float loss_;
    std::shared_ptr<TpService>  tp_;
    std::mutex mutex_;
    std::queue<Packet> queue_;
    boost::asio::io_service* ioService_;
//    boost::thread_group threadpool_;
//    boost::scoped_ptr<boost::asio::io_service::work> work_;
};

#endif // SHAPER_HPP

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
#include <queue>

class Shaper : public Filter
{
 public:
    Shaper();
    ~Shaper();
    void handlePacket(Packet pkt);
    void queueTimeEvent();

 private:
    
    std::queue<Packet> queue_;
    boost::asio::io_service ioService_;
    boost::thread_group threadpool_;
    boost::scoped_ptr<boost::asio::io_service::work> work_;
};

#endif // SHAPER_HPP

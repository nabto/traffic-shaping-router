#ifndef SHAPER_HPP
#define SHAPER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <filter.hpp>
#include <packet.hpp>
#include <tp_service.hpp>
#include <async_queue.hpp>
#include <queue>
#include <mutex>

// Delay class imposing delays on all packets
class StaticDelay : public Filter, public std::enable_shared_from_this<StaticDelay>
{
 public:
    StaticDelay();
    ~StaticDelay();
    bool init();
    // Packet handler putting packets into async queue and returning
    void handlePacket(std::shared_ptr<ParentPacket> pkt);
    // PopHandler pops packets from the async queue and delays them
    void popHandler(const std::error_code& ec, const std::shared_ptr<ParentPacket> pkt);
    // set function for configuration
    void setDelay(int delay) {delay_ = boost::posix_time::milliseconds(delay);}
 private:
    boost::posix_time::time_duration delay_;
    AsyncQueue<std::shared_ptr<ParentPacket> > queue_;
    boost::asio::io_service* ioService_;
};

class Loss : public Filter, public std::enable_shared_from_this<Loss>
{
 public:
    Loss(): loss_(0) {srand (static_cast <unsigned> (time(0)));}
    ~Loss() {}
    // Packet handler which droppes packets randomly
    void handlePacket(std::shared_ptr<ParentPacket> pkt);
    // set function to configure probability of packet loss
    void setLoss(float loss){loss_ = loss;}

 private:
    float loss_;
};

#endif // SHAPER_HPP

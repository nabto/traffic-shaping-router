#ifndef DYNAMIC_LOSS_HPP
#define DYNAMIC_LOSS_HPP

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <filter.hpp>
#include <packet.hpp>
#include <tp_service.hpp>

class DynamicLoss : public Filter, public std::enable_shared_from_this<DynamicLoss>
{
 public:
    DynamicLoss();
    ~DynamicLoss();
    bool init();
    void handlePacket(std::shared_ptr<ParentPacket> pkt);
    bool setLosses(std::vector<int> times, std::vector<float> losses);
 private:
    void handleTimeout();
    float currentLoss_;
    size_t currentIndex_;
    
    std::vector<int> times_;
    std::vector<float> losses_;
    
    boost::posix_time::time_duration timer_;
    boost::asio::io_service* ioService_;
};

#endif // DYNAMIC_LOSS_HPP

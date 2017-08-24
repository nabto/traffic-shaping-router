#ifndef DYNAMIC_DELAY_HPP
#define DYNAMIC_DELAY_HPP


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
class DynamicDelay : public Filter, public std::enable_shared_from_this<DynamicDelay>
{
 public:
    DynamicDelay();
    ~DynamicDelay();
    bool init();
    void handlePacket(PacketPtr pkt);
    void popHandler(const std::error_code& ec, const PacketPtr pkt);
    void delayPopHandler(const std::error_code& ec, const int d);

    /* Defining the delay curve used by the filter.
     * delays is a vector containing delay values in ms
     * resolution is the time resolution in seconds
     * delay values are the delay values at time n*resolution
     * delay values between data points are extrapolated linearly at 1 sec. resolution
     * linear extrapolation resulting in non-integer values are floored
     * ex:
     * setDelays([0, 8, 5], 4)
     * means the delay values are separated by 4 seconds,
     * so at t=0 d=0; t=4 d=8; t=8 d=5; t=12 d=0; t=16 d=8...
     * the resulting 1 second resolution curve is:
     * [0, 2, 4, 6, 8, 7, 6, 5, 5, 3, 2, 1]
     * @return the resulting 1 second resulution curve to be emulated
     */
    std::vector<int> setDelays(std::vector<int> delays, int resolution);
 private:
    std::mutex mutex_;
    boost::posix_time::time_duration delay_;
    boost::posix_time::time_duration int_;
    AsyncQueue<PacketPtr> queue_;
    AsyncQueue<int> delayQ_;
    boost::asio::io_service* ioService_;
};


#endif // DYNAMIC_DELAY_HPP

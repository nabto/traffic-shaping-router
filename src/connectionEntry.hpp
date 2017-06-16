
#include <memory>
#include "tpService.hpp"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <mutex>

class ConnectionEntry;
typedef std::shared_ptr<ConnectionEntry> ConnectionEntryPtr;
typedef std::weak_ptr<ConnectionEntry> ConnectionEntryWeakPtr;

// Class used to timeout connections in the Nat class
class ConnectionEntry : public std::enable_shared_from_this<ConnectionEntry>
{
 public:
    ConnectionEntry(int timeout): timer_(*(TpService::getInstance()->getIoService())), timeout_(timeout) {}
    void start(){ armTimer();}
    void rearm(){ stop(); armTimer();}
    void stop(){
        boost::system::error_code ec;
        timer_.cancel(ec);
    }

 private:
    boost::asio::steady_timer timer_;
    int timeout_;
    std::mutex mutex_;
    
    void armTimer() {
        std::lock_guard<std::mutex> lock(mutex_);
        boost::system::error_code ec;
        timer_.expires_from_now(std::chrono::seconds(timeout_), ec);
        auto self = shared_from_this();
        timer_.async_wait([self](const boost::system::error_code&)
                          {
                              return;
                          });
    }       


};

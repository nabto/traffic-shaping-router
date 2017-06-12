
#include <memory>
#include "tpService.hpp"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <mutex>

class ConnectionEntry;
typedef std::shared_ptr<ConnectionEntry> ConnectionEntryPtr;
typedef std::weak_ptr<ConnectionEntry> ConnectionEntryWeakPtr;


class ConnectionEntry : public std::enable_shared_from_this<ConnectionEntry>
{
 public:
    ConnectionEntry(int timeout, PacketPtr pkt): timer_(*(TpService::getInstance()->getIoService())), timeout_(timeout), srcIp_(pkt->getSourceIP()), dstIp_(pkt->getDestinationIP()), sport_(pkt->getSourcePort()), dport_(pkt->getDestinationPort()), proto_(pkt->getProtocol()) {}
    void start(){ armTimer();}
    void rearm(){ stop(); armTimer();}
    void stop(){
        boost::system::error_code ec;
        timer_.cancel(ec);
    }

    inline bool operator==(const ConnectionEntry& ct){
        return (std::make_tuple(srcIp_, dstIp_, sport_, dport_, proto_) == std::make_tuple(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    inline bool operator!=(const ConnectionEntry& ct){
        return (std::make_tuple(srcIp_, dstIp_, sport_, dport_, proto_) != std::make_tuple(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
 private:
    boost::asio::steady_timer timer_;
    int timeout_;
    uint32_t srcIp_;
    uint32_t dstIp_;
    uint16_t sport_;
    uint16_t dport_;
    uint8_t proto_;
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

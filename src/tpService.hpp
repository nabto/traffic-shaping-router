#ifndef TPSERVICE_HPP
#define TPSERVICE_HPP

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Thread pool service supplying worker threads for async operations
class TpService : public std::enable_shared_from_this<TpService>
{
 public:
    static std::shared_ptr<TpService> getInstance();
    TpService();
    ~TpService();
    void init(int size);
    boost::asio::io_service* getIoService(){return &ioService_;}
    
 private:
    boost::asio::io_service ioService_;
    boost::thread_group threadpool_;
    boost::scoped_ptr<boost::asio::io_service::work> work_;
};


#endif // TPSERVICE_HPP

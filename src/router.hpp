#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <memory>
#include "packet.hpp"

class Router;
typedef std::shared_ptr<Router> RouterPtr;

class Router : public std::enable_shared_from_this<Router>
{
 public:
    static RouterPtr getInstance();
    Router();
    ~Router();
    bool init();
    bool execute();
    int handlePacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);
    void packetCallback(Packet* pkt);
 private:
    std::mutex mutex_;
    int id_;
    std::vector<Packet> packets_;
    boost::asio::io_service ioService_;
    boost::thread_group threadpool_;
    boost::scoped_ptr<boost::asio::io_service::work> work_;
};

#endif // ROUTER_HPP

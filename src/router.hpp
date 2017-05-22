#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <memory>
#include <mutex>
#include "packet.hpp"
#include "filter.hpp"
#include "shaper.hpp"
#include "nat.hpp"


class Router;
typedef std::shared_ptr<Router> RouterPtr;

class Router : public Filter
{
 public:
    static RouterPtr getInstance();
    Router();
    ~Router();
    void init();
    bool execute();
    int newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);
    void handlePacket(Packet pkt);
 private:
    std::mutex mutex_;
    int id_;
    std::vector<Packet> packets_;
    std::shared_ptr<Filter> shaper_;
    std::shared_ptr<Filter> nat_;
    //Shaper shaper_;
    //Nat nat_;
};

#endif // ROUTER_HPP

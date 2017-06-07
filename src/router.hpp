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
    void handlePacket(PacketPtr pkt);
    void setDelay(int del){delayMs_ = del;}
    void setLoss(float loss){lossProb_ = loss;}
    void setOutIf(std::string ifOut){ifOut_ = ifOut;}
    
 private:
    std::vector<Packet> packets_;
    std::shared_ptr<Filter> delay_;
    std::shared_ptr<Filter> loss_;
    std::shared_ptr<Filter> nat_;
    int delayMs_;
    float lossProb_;
    std::string ifOut_;
};

#endif // ROUTER_HPP

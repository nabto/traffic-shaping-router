#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <memory>
#include <mutex>
#include "packet.hpp"
#include "filter.hpp"
#include "shaper.hpp"
#include "nat.hpp"
#include "output.hpp"

class Router;
typedef std::shared_ptr<Router> RouterPtr;

class Router : public Filter, public std::enable_shared_from_this<Router>
{
 public:
    static RouterPtr getInstance();
    Router();
    ~Router();
    void init();
    bool execute();
    int newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);
    void handlePacket(PacketPtr pkt);
    
    void setDelay(int del){delay_->setDelay(del);}
    void setLoss(float loss){loss_->setLoss(loss);}
    void setIPs(std::string ipExt, std::string ipInt){nat_->setIPs(ipExt, ipInt);}
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort){nat_->setDnatRule(ip, extPort, intPort);}
    
 private:
    std::shared_ptr<StaticDelay> delay_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Nat> nat_;
    std::shared_ptr<Output> output_;
};

#endif // ROUTER_HPP

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
    void setDelay(int del){delayMs_ = del;}
    void setLoss(float loss){lossProb_ = loss;}
    void setExtIf(std::string ifExt){ifExt_ = ifExt;}
    void setExtIp(std::string ipExt){ipExt_ = ipExt;}
    void setIntIf(std::string ifInt){ifInt_ = ifInt;}
    void setIntIp(std::string ipInt){ipInt_ = ipInt;}

    
 private:
    std::vector<Packet> packets_;
    std::shared_ptr<StaticDelay> delay_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Nat> nat_;
    int delayMs_;
    float lossProb_;
    std::string ifExt_;
    std::string ipExt_;
    std::string ifInt_;
    std::string ipInt_;
};

#endif // ROUTER_HPP

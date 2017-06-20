#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <memory>
#include <mutex>
#include "packet.hpp"
#include "filter.hpp"
#include "shaper.hpp"
#include "nat.hpp"
#include "burst_filter.hpp"
#include "output.hpp"


//// Singleton router class handling incoming packets from nf queue
class Router;
typedef std::shared_ptr<Router> RouterPtr;

class Router : public Filter, public std::enable_shared_from_this<Router>
{
 public:
    static RouterPtr getInstance();
    Router();
    ~Router();
    void init();
    /* Method executing the router
     * This starts an infinite while loop handling packets
     * This function should never return
     */
    bool execute();

    // Callback function for nf queue
    int newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);

    // set functions configuring the filters 
    void setDelay(int del) {delay_->setDelay(del);}
    void setLoss(float loss) {loss_->setLoss(loss);}
    void setIPs(std::string ipExt, std::string ipInt) {nat_->setIPs(ipExt, ipInt);}
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort) {nat_->setDnatRule(ip, extPort, intPort);}
    void setBurstDuration(int dur) {burst_->setBurstDuration(dur);}
    void setSleepDuration(int dur) {burst_->setSleepDuration(dur);}

    // Since router is the first filter, handlePacket is not used
    void handlePacket(PacketPtr pkt);
    
 private:
    std::shared_ptr<StaticDelay> delay_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Nat> nat_;
    std::shared_ptr<Burst> burst_;
    std::shared_ptr<Output> output_;
};

#endif // ROUTER_HPP

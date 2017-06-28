#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <memory>
#include <mutex>
#include "packet.hpp"
#include "filter.hpp"
#include <filters/shaper.hpp>
#include <filters/nat.hpp>
#include <filters/burst_filter.hpp>
#include <filters/tbf_filter.hpp>
#include <filters/output.hpp>


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
    void init(std::vector<std::string> filters);
    /* Method executing the router
     * This starts an infinite while loop handling packets
     * This function should never return
     */
    bool execute();

    // Callback function for nf queue
    int newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);

    // set functions configuring the filters 
    void setDelay(int del) {if(delay_){delay_->setDelay(del);}}
    void setLoss(float loss) {if(loss_){loss_->setLoss(loss);}}
    void setIPs(std::string ipExt, std::string ipInt) {if(nat_){nat_->setIPs(ipExt, ipInt);}}
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort) {if(nat_){nat_->setDnatRule(ip, extPort, intPort);}}
    void setBurstDuration(int dur) {if(burst_){burst_->setBurstDuration(dur);}}
    void setSleepDuration(int dur) {if(burst_){burst_->setSleepDuration(dur);}}
    void setTbfRateLimit(uint32_t rate) {if(tbf_){tbf_->setRateLimit(rate);}}
    void setTbfMaxTokens(uint32_t t) {if(tbf_){tbf_->setMaxTokens(t);}}
    void setTbfMaxPackets(uint32_t p) {if(tbf_){tbf_->setMaxPackets(p);}}
    void setTbfRedStart(uint32_t s) {if(tbf_){tbf_->setRedStart(s);}}
    void setTbfRedDrop(float d) {if(tbf_){tbf_->setRedDrop(d);}}
    
    // Since router is the first filter, handlePacket is not used
    void handlePacket(PacketPtr pkt);
    
 private:
    std::shared_ptr<StaticDelay> delay_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Nat> nat_;
    std::shared_ptr<Burst> burst_;
    std::shared_ptr<TokenBucketFilter> tbf_;
    std::shared_ptr<Output> output_;
    bool initialized_;
};

#endif // ROUTER_HPP

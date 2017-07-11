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
    // StaticDelay filter
    void setDelay(int del) {if(delay_){delay_->setDelay(del);}}
    // Loss filter
    void setLoss(float loss) {if(loss_){loss_->setLoss(loss);}}
    // Nat filter
    void setIPs(std::string ipExt, std::string ipInt) {extIp_ = inet_network(ipExt.c_str()); if(nat_){nat_->setIPs(ipExt, ipInt);}}
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort) {if(nat_){nat_->setDnatRule(ip, extPort, intPort);}}
    void setNatType(std::string type) {if(nat_){nat_->setNatType(type);}};
    // Burst filter
    void setBurstDuration(int dur) {if(burst_){burst_->setBurstDuration(dur);}}
    void setSleepDuration(int dur) {if(burst_){burst_->setSleepDuration(dur);}}
    // TokenBucketFilter filter
    void setTbfInRateLimit(uint32_t rate) {if(tbf_){tbf_->setInRateLimit(rate);}}
    void setTbfInMaxTokens(uint32_t t) {if(tbf_){tbf_->setInMaxTokens(t);}}
    void setTbfInMaxPackets(uint32_t p) {if(tbf_){tbf_->setInMaxPackets(p);}}
    void setTbfInRedStart(uint32_t s) {if(tbf_){tbf_->setInRedStart(s);}}
    void setTbfOutRateLimit(uint32_t rate) {if(tbf_){tbf_->setOutRateLimit(rate);}}
    void setTbfOutMaxTokens(uint32_t t) {if(tbf_){tbf_->setOutMaxTokens(t);}}
    void setTbfOutMaxPackets(uint32_t p) {if(tbf_){tbf_->setOutMaxPackets(p);}}
    void setTbfOutRedStart(uint32_t s) {if(tbf_){tbf_->setOutRedStart(s);}}
    
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
    uint32_t extIp_;
};

#endif // ROUTER_HPP

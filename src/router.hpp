#ifndef ROUTER_HPP
#define ROUTER_HPP
#include <memory>
#include <mutex>
#include "packet.hpp"
#include "packet6.hpp"
#include "filter.hpp"
#include <filters/shaper.hpp>
#include <filters/dynamic_delay.hpp>
#include <filters/dynamic_loss.hpp>
#include <filters/nat.hpp>
#include <filters/burst_filter.hpp>
#include <filters/tbf_filter.hpp>
#include <filters/output.hpp>
#include <filters/output6_libnet.hpp>
#include <filters/output_libnet.hpp>
#include <filters/input.hpp>
#include <filters/input_drop.hpp>


//// Singleton router class handling incoming packets from nf queue
class Router;
typedef std::shared_ptr<Router> RouterPtr;

class Router : public Filter, public std::enable_shared_from_this<Router>
{
 public:
    static RouterPtr getInstance();
    Router();
    ~Router();
    bool init(std::string extIf);
    bool init(std::string extIf, std::vector<std::string> filters);
    /* Method executing the router
     * This starts an infinite while loop handling packets
     * This function should never return
     */
    bool execute();
    bool execute6();

    // Callback function for nf queue
    int newPacket(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);
    int newPacket6(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);

    // set functions configuring the filters 
    // StaticDelay filter
    void setDelay(int del) {
        if(delay_){delay_->setDelay(del);}
        if(delay6_){delay6_->setDelay(del);}
    }
    // Loss filter
    void setLoss(float loss) {
        if(loss_){loss_->setLoss(loss);}
        if(loss6_){loss6_->setLoss(loss);}
    }
    // Nat filter
    void setIPs(std::string ipExt, std::string ipInt) {
        if(nat_){nat_->setIPs(ipExt, ipInt);}
    }
    void setDnatRule(std::string ip, uint16_t extPort, uint16_t intPort) {
        if(nat_){nat_->setDnatRule(ip, extPort, intPort);}
    }
    void setNatType(std::string type) {
        if(nat_){nat_->setNatType(type);}
    }
    void setNatTimeout(uint32_t to) {
        if(nat_){nat_->setTimeout(to);}
    }
    // Burst filter
    void setBurstDuration(int dur) {
        if(burst_){burst_->setBurstDuration(dur);}
        if(burst6_){burst6_->setBurstDuration(dur);}
    }
    void setSleepDuration(int dur) {
        if(burst_){burst_->setSleepDuration(dur);}
        if(burst6_){burst6_->setSleepDuration(dur);}
    }
    // TokenBucketFilter filter
    void setTbfInRateLimit(uint32_t rate) {
        if(tbf_){tbf_->setInRateLimit(rate);}
        if(tbf6_){tbf6_->setInRateLimit(rate);}
    }
    void setTbfInMaxTokens(uint32_t t) {
        if(tbf_){tbf_->setInMaxTokens(t);}
        if(tbf6_){tbf6_->setInMaxTokens(t);}
    }
    void setTbfInMaxPackets(uint32_t p) {
        if(tbf_){tbf_->setInMaxPackets(p);}
        if(tbf6_){tbf6_->setInMaxPackets(p);}
    }
    void setTbfInRedStart(uint32_t s) {
        if(tbf_){tbf_->setInRedStart(s);}
        if(tbf6_){tbf6_->setInRedStart(s);}
    }
    void setTbfOutRateLimit(uint32_t rate) {
        if(tbf_){tbf_->setOutRateLimit(rate);}
        if(tbf6_){tbf6_->setOutRateLimit(rate);}
    }
    void setTbfOutMaxTokens(uint32_t t) {
        if(tbf_){tbf_->setOutMaxTokens(t);}
        if(tbf6_){tbf6_->setOutMaxTokens(t);}
    }
    void setTbfOutMaxPackets(uint32_t p) {
        if(tbf_){tbf_->setOutMaxPackets(p);}
        if(tbf6_){tbf6_->setOutMaxPackets(p);}
    }
    void setTbfOutRedStart(uint32_t s) {
        if(tbf_){tbf_->setOutRedStart(s);}
        if(tbf6_){tbf6_->setOutRedStart(s);}
    }
    // DynamicDelay filter
    void setDynamicDelays(std::vector<int> d, int r){
        if(dynDel_){dynDel_->setDelays(d,r);}
        if(dynDel6_){dynDel6_->setDelays(d,r);}
    }
    // DynamicLoss filter
    void setDynamicLosses(std::vector<int> t, std::vector<float> l){
        if(dynLoss_){dynLoss_->setLosses(t,l);}
        if(dynLoss6_){dynLoss6_->setLosses(t,l);}
    }
    
    // Since router is the first filter, handlePacket is not used
    void handlePacket(std::shared_ptr<ParentPacket> pkt);
    
 private:
    bool initFilters(struct nfq_q_handle *qh);
    bool init6Filters(struct nfq_q_handle *qh);
    
    bool initialized_;
    std::string extIf_;
    

    // IPv4 chain
    std::shared_ptr<Filter> start_;
    std::shared_ptr<Input> input_;
    std::shared_ptr<InputDrop> inputDrop_;
    std::shared_ptr<StaticDelay> delay_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Nat> nat_;
    std::shared_ptr<Burst> burst_;
    std::shared_ptr<TokenBucketFilter> tbf_;
    std::shared_ptr<DynamicDelay> dynDel_;
    std::shared_ptr<DynamicLoss> dynLoss_;
    std::shared_ptr<OutputLibnet> outputLibnet_;
    std::shared_ptr<Output> output_;

    // IPv6 chain
    std::shared_ptr<Filter> start6_;
    std::shared_ptr<Input> input6_;
    std::shared_ptr<InputDrop> inputDrop6_;
    std::shared_ptr<StaticDelay> delay6_;
    std::shared_ptr<Loss> loss6_;
    std::shared_ptr<Burst> burst6_;
    std::shared_ptr<TokenBucketFilter> tbf6_;
    std::shared_ptr<DynamicDelay> dynDel6_;
    std::shared_ptr<DynamicLoss> dynLoss6_;
    std::shared_ptr<Output6Libnet> output6Libnet_;
    std::shared_ptr<Output> output6_;
};

#endif // ROUTER_HPP

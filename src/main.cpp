#include "router.hpp"
#include <stdlib.h>
#include <iostream>
#include <boost/program_options.hpp>

namespace po=boost::program_options;

int main (int argc, char* argv[])
{
    std::string extIp;
    std::string intIp;
    std::string natType;
    std::string extIf;
    int del = 0;
    float los = 0;
    int burstDur = 0;
    int sleepDur = 0;
    std::vector<uint16_t> extNat;
    std::vector<uint16_t> intNat;
    po::options_description desc("Allowed options");
    RouterPtr rt = Router::getInstance();

    desc.add_options()
        ("help,h", "Print usage message")
        ("ext-if", po::value<std::string>()->composing(), "The external interface of the router node, used to destinquish ingoing from outgoing traffic.")
        ("nat-ext-ip", po::value<std::string>()->composing(), "The ip of the external interface of the router (Required with Nat filter)")
        ("nat-int-ip", po::value<std::string>()->composing(), "The ip of the internal interface of the router (Required with Nat filter)")
        ("nat-type", po::value<std::string>()->composing(), "The type of NAT to be applied, valid types: portr, addrr, fullcone, symnat")
        ("nat-timeout", po::value<uint32_t>()->composing(), "The timeout of NAT connections in seconds, 10min default")
        ("delay,d", po::value<int>()->composing(), "The network delay in ms to be imposed")
        ("loss,l", po::value<float>()->composing(), "The packet loss probability")
        ("tbf-in-rate", po::value<int>()->composing(), "The rate limit in kbit pr. sec. for the token bucket filter")
        ("tbf-in-max-tokens", po::value<int>()->composing(), "The maximum amount of tokens for the token bucket filter")
        ("tbf-in-max-packets", po::value<int>()->composing(), "The packet queue length of the token bucket filter")
        ("tbf-in-red-start", po::value<int>()->composing(), "The packet queue length at which random early drop kick in")
        ("tbf-out-rate", po::value<int>()->composing(), "The rate limit in kbit pr. sec. for the token bucket filter")
        ("tbf-out-max-tokens", po::value<int>()->composing(), "The maximum amount of tokens for the token bucket filter")
        ("tbf-out-max-packets", po::value<int>()->composing(), "The packet queue length of the token bucket filter")
        ("tbf-out-red-start", po::value<int>()->composing(), "The packet queue length at which random early drop kick in")
        ("dyn-delays", po::value<std::vector<int> >()->multitoken(), "Delay values for the dynamic delay filter")
        ("dyn-delay-res", po::value<int>()->composing(), "The time resolution of the delay values for delay filter")
        ("dyn-losses", po::value<std::vector<float> >()->multitoken(), "Loss values for the dynamic loss filter")
        ("dyn-loss-times", po::value<std::vector<int> >()->multitoken(), "time values for the dynamic loss filter")
        ("burst-dur", po::value<int>()->composing(), "The time in ms when packets are collected for bursts")
        ("burst-sleep", po::value<int>()->composing(), "The time in ms between bursts")
        ("nat-ext-port,e", po::value<std::vector<uint16_t> >()->multitoken(),"External port numbers for port forwarding")
        ("nat-int-port,i", po::value<std::vector<uint16_t> >()->multitoken(),"Internal port numbers for port forwarding, if defined must have same length as nat-ext-port, if not defined nat-ext-port will be reused for internal ports")
        ("filters" , po::value<std::vector<std::string> >()->multitoken(),"Ordered list of filters to use, valid filters are: StaticDelay, Loss, Nat, Burst, TokenBucketFilter, DynamicDelay, DynamicLoss")
        ("ipv6", "use IPv6 instead of IPv4")
        ("accept-mode", "run the router in accept mode instead of drop mode")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (...) {
        std::cerr << "Illegal options!\nUsage: router [options]\n" << desc;
        exit(1);
    }

    if (vm.count("help")){
        std::cerr << desc << std::endl;
        exit(0);
    }

    if (vm.count("ext-if")){
        extIf = vm["ext-if"].as<std::string>();
    } else {
        std::cerr << "The external interface is required\n" << desc;
        exit(1);
    }
    
    if (vm.count("filters")){
        std::cout << "using selected filters " << std::endl;
        std::vector<std::string> filters = vm["filters"].as<std::vector<std::string> >();
        if (vm.count("accept-mode")){
            filters.insert(filters.begin(), "Input");
            filters.push_back("Output");
        } else {
            filters.insert(filters.begin(), "InputDrop");
            filters.push_back("OutputLibnet");
        }
        rt->init(extIf, filters);
    } else {
        rt->init(extIf);
    }
    
    if (vm.count("nat-ext-ip")){
        extIp = vm["nat-ext-ip"].as<std::string>();
    } else {
        std::cout << "WARNING: nat-ext-ip was not set, this only works if the Nat filter is unused!" << std::endl;
    }
    
    if (vm.count("nat-int-ip")){
        intIp = vm["nat-int-ip"].as<std::string>();
    } else {
        std::cout << "WARNING: nat-int-ip was not set, this only works if the Nat filter is unused!" << std::endl;
    }

    rt->setIPs(extIp,intIp);
    
    if (vm.count("nat-type")){
        natType = vm["nat-type"].as<std::string>();
        rt->setNatType(natType);
    }
    if (vm.count("nat-timeout")){
        uint32_t natTimeout = vm["nat-timeout"].as<uint32_t>();
        rt->setNatTimeout(natTimeout);
    }

    if (vm.count("delay")){
        del = vm["delay"].as<int>();
        rt->setDelay(del);
    }
    
    if (vm.count("loss")){
        los = vm["loss"].as<float>();
        rt->setLoss(los);
    }

    if (vm.count("tbf-in-rate")){
       int rate  = vm["tbf-in-rate"].as<int>();
       rt->setTbfInRateLimit(rate);
    }

    if (vm.count("tbf-in-max-tokens")){
        int tbfTokens = vm["tbf-in-max-tokens"].as<int>();
        rt->setTbfInMaxTokens(tbfTokens);
    }

    if (vm.count("tbf-in-max-packets")){
        int tbfPackets = vm["tbf-in-max-packets"].as<int>();
        rt->setTbfInMaxPackets(tbfPackets);
    }

    if (vm.count("tbf-in-red-start")){
        int tbfRedStart = vm["tbf-in-red-start"].as<int>();
        rt->setTbfInRedStart(tbfRedStart);
    }

    if (vm.count("tbf-out-rate")){
       int rate  = vm["tbf-out-rate"].as<int>();
       rt->setTbfOutRateLimit(rate);
    }

    if (vm.count("tbf-out-max-tokens")){
        int tbfTokens = vm["tbf-out-max-tokens"].as<int>();
        rt->setTbfOutMaxTokens(tbfTokens);
    }

    if (vm.count("tbf-out-max-packets")){
        int tbfPackets = vm["tbf-out-max-packets"].as<int>();
        rt->setTbfOutMaxPackets(tbfPackets);
    }

    if (vm.count("tbf-out-red-start")){
        int tbfRedStart = vm["tbf-out-red-start"].as<int>();
        rt->setTbfOutRedStart(tbfRedStart);
    }

    if (vm.count("burst-dur")){
       burstDur  = vm["burst-dur"].as<int>();
       rt->setBurstDuration(burstDur);
    }

    if (vm.count("burst-sleep")){
        sleepDur = vm["burst-sleep"].as<int>();
        rt->setSleepDuration(sleepDur);
    }

    if (vm.count("dyn-delays")){
        if (vm.count("dyn-delay-res")){
            std::vector<int> dd = vm["dyn-delays"].as<std::vector<int> >();
            int ddr = vm["dyn-delay-res"].as<int>();
            rt->setDynamicDelays(dd,ddr);
        } else {
            std::cerr << "dyn-delays where set without dyn-delay-res" << std::endl << desc << std::endl;
            exit(1);
        }
    }

    if (vm.count("dyn-losses")){
        if (vm.count("dyn-loss-times")){
            std::vector<float> dl = vm["dyn-losses"].as<std::vector<float> >();
            std::vector<int> dlt = vm["dyn-loss-times"].as<std::vector<int>>();
            if (dl.size() != dlt.size()) {
                std::cerr << "dyn-losses and dyn-loss-times did not have matching lengths" << std::endl << desc << std::endl;
                exit(1);
            }
            rt->setDynamicLosses(dlt,dl);
        } else {
            std::cerr << "dyn-losses where set without dyn-loss-times" << std::endl << desc << std::endl;
            exit(1);
        }
    }

    if (vm.count("nat-ext-port")){
        if (vm.count("nat-int-port")){
            extNat = vm["nat-ext-port"].as<std::vector<uint16_t> >();
            intNat = vm["nat-int-port"].as<std::vector<uint16_t> >();
        } else {
            extNat = vm["nat-ext-port"].as<std::vector<uint16_t> >();
            intNat = vm["nat-ext-port"].as<std::vector<uint16_t> >();
        }
        if(extNat.size() != intNat.size()){
            std::cerr << "nat-ext-port was not same length as nat-int-port" << std::endl << desc << std::endl;
            exit(1);
        }
    }

    for(size_t i = 0; i < extNat.size(); i++){
        rt->setDnatRule(intIp, extNat[i], intNat[i]);
    }

    std::cout << "Router started with delay: " << del << ", loss: " << los << ", int IP: " << intIp << ", and ext IP " << extIp << std::endl;
    if (vm.count("ipv6")){
        rt->execute6();
    } else { 
        rt->execute();
    }

}

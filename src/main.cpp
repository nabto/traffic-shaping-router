#include "router.hpp"
#include <stdlib.h>
#include <iostream>
#include <boost/program_options.hpp>

namespace po=boost::program_options;

int main (int argc, char* argv[])
{
    std::string extIp;
    std::string intIp;
    int del = 0;
    float los = 0;
    std::vector<uint16_t> extNat;
    std::vector<uint16_t> intNat;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print usage message")
        ("ext_ip", po::value<std::string>()->composing(), "The ip of the external interface of the router")
        ("int_ip", po::value<std::string>()->composing(), "The ip of the internal interface of the router")
        ("del,d", po::value<int>()->composing(), "The network delay in ms to be imposed")
        ("loss,l", po::value<float>()->composing(), "The packet loss probability")
        ("ext_nat,e", po::value<std::vector<uint16_t> >()->multitoken(),"External port numbers for port forwarding")
        ("int_nat,i", po::value<std::vector<uint16_t> >()->multitoken(),"Internal port numbers for port forwarding, if defined must have same length as ext_nat, if not defined ext_nat will be reused for internal ports")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    } catch (...) {
        std::cerr << "Illegal options!\nUsage: basestation [options]\n" << desc;
        exit(1);
    }

    if (vm.count("help")){
        std::cerr << desc << std::endl;
        exit(0);
    }

    if (vm.count("ext_ip")){
        extIp = vm["ext_ip"].as<std::string>();
    } else {
        std::cerr << "ext_ip is required" << std::endl << desc << std::endl;
        exit(1);
    }
    
    if (vm.count("int_ip")){
        intIp = vm["int_ip"].as<std::string>();
    } else {
        std::cerr << "int_ip is required" << std::endl << desc << std::endl;
        exit(1);
    }
    
    if (vm.count("del")){
        del = vm["del"].as<int>();
    } else {
        del = 0;
    }
    
    if (vm.count("loss")){
        los = vm["loss"].as<float>();
    } else {
        los = 0;
    }

    if (vm.count("ext_nat")){
        if (vm.count("int_nat")){
            extNat = vm["ext_nat"].as<std::vector<uint16_t> >();
            intNat = vm["int_nat"].as<std::vector<uint16_t> >();
        } else {
            extNat = vm["ext_nat"].as<std::vector<uint16_t> >();
            intNat = vm["ext_nat"].as<std::vector<uint16_t> >();
        }
        if(extNat.size() != intNat.size()){
            std::cerr << "ext_nat was not same length as int_nat" << std::endl << desc << std::endl;
            exit(1);
        }
    }
    std::cout << "Router started with delay: " << del << ", loss: " << los << ", int IP: " << intIp << ", and ext IP " << extIp << std::endl;
    RouterPtr rt = Router::getInstance();
    
    rt->setIPs(extIp,intIp);
    rt->setDelay(del);
    rt->setLoss(los);
    for(size_t i = 0; i < extNat.size(); i++){
        rt->setDnatRule(intIp, extNat[i], intNat[i]);
    }
    
    rt->execute();

}

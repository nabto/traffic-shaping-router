#include "router.hpp"
#include <stdlib.h>
#include <iostream>
#include <boost/program_options.hpp>

namespace po=boost::program_options;

int main (int argc, char* argv[])
{
    std::string extIf;
    std::string extIp;
    std::string intIf;
    std::string intIp;
    int del = 0;
    float los = 0;
    std::vector<uint16_t> extNat;
    std::vector<uint16_t> intNat;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print usage message")
        ("ext_if", po::value<std::string>()->composing(), "The external interface of the router")
        ("ext_ip", po::value<std::string>()->composing(), "The ip of the external interface of the router")
        ("int_if", po::value<std::string>()->composing(), "The internal interface of the router")
        ("int_ip", po::value<std::string>()->composing(), "The ip of the internal interface of the router")
        ("del,d", po::value<int>()->composing(), "The network delay to be imposed")
        ("loss,l", po::value<float>()->composing(), "The packet loss probability")
        ("ext_nat,e", po::value<std::vector<uint16_t> >()->multitoken(),"External port numbers for port forwarding, if defined must have same length as int_nat")
        ("int_nat,i", po::value<std::vector<uint16_t> >()->multitoken(),"Internal port numbers for port forwarding, if defined must have same length as ext_nat")
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

    if (vm.count("ext_if")){
        extIf = vm["ext_if"].as<std::string>();
    } else {
        std::cerr << "ext_if is required" << std::endl << desc << std::endl;
        exit(1);
    }
    
    if (vm.count("ext_ip")){
        extIp = vm["ext_ip"].as<std::string>();
    } else {
        std::cerr << "ext_ip is required" << std::endl << desc << std::endl;
        exit(1);
    }
    
    if (vm.count("int_if")){
        intIf = vm["int_if"].as<std::string>();
    } else {
        std::cerr << "int_if is required" << std::endl << desc << std::endl;
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
        std::cerr << "del is required" << std::endl << desc << std::endl;
        exit(1);
    }
    
    if (vm.count("loss")){
        los = vm["loss"].as<float>();
    } else {
        std::cerr << "loss is required" << std::endl << desc << std::endl;
        exit(1);
    }

    if (vm.count("ext_nat")){
        if (vm.count("int_nat")){
            extNat = vm["ext_nat"].as<std::vector<uint16_t> >();
            intNat = vm["int_nat"].as<std::vector<uint16_t> >();
        } else {
            std::cerr << "ext_nat was set without int_nat, they must have same length" << std::endl << desc << std::endl;
            exit(1);
        }
        if(extNat.size() != intNat.size()){
            std::cerr << "ext_nat was not same length as int_nat" << std::endl << desc << std::endl;
            exit(1);
        }
    }
    std::cout << "Router started with delay: " << del << ", loss: " << los << ", and IF " << extIf << std::endl;
    RouterPtr rt = Router::getInstance();
    
    rt->setIPs(extIp,intIp);
    rt->setDelay(del);
    rt->setLoss(los);
    for(size_t i = 0; i < extNat.size(); i++){
        rt->setDnatRule(intIp, extNat[i], intNat[i]);
    }
    
    rt->execute();

}

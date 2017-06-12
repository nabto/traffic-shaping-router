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
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print usage message")
        // ("ext_if", po::value<std::string>(&extIf), "The external interface of the router")
        // ("ext_ip", po::value<std::string>(&extIp), "The ip of the external interface of the router")
        // ("int_if", po::value<std::string>(&intIf), "The internal interface of the router")
        // ("int_ip", po::value<std::string>(&intIp), "The ip of the internal interface of the router")
        // ("del,d", po::value<int>(&del), "The network delay to be imposed")
        // ("loss,l", po::value<float>(&los), "The packet loss probability")
        ("ext_if", po::value<std::string>()->composing(), "The external interface of the router")
        ("ext_ip", po::value<std::string>()->composing(), "The ip of the external interface of the router")
        ("int_if", po::value<std::string>()->composing(), "The internal interface of the router")
        ("int_ip", po::value<std::string>()->composing(), "The ip of the internal interface of the router")
        ("del,d", po::value<int>()->composing(), "The network delay to be imposed")
        ("loss,l", po::value<float>()->composing(), "The packet loss probability")
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
    
    std::cout << "Router started with delay: " << del << ", loss: " << los << ", and IF " << extIf << std::endl;
    RouterPtr rt = Router::getInstance();
    rt->setDelay(del);
    rt->setLoss(los);
    rt->setExtIf(extIf);
    rt->setExtIp(extIp);
    rt->setIntIf(intIf);
    rt->setIntIp(intIp);
    rt->init();
    rt->execute();

}

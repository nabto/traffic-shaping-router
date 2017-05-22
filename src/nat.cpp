#include "nat.hpp"
#include "packet.hpp"
#include "structures.h"


Nat::Nat(){}
Nat::~Nat(){}
void Nat::handlePacket(Packet pkt){
    std::cout << "nat handling" << std::endl;
    pkt.setVerdict(ACCEPTED);
    std::cout << "verdict accepted" << std::endl;
    next_->handlePacket(pkt);
    std::cout << "returning" << std::endl;
}


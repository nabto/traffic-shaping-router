#include "nat.hpp"
#include "packet.hpp"
#include "structures.h"


Nat::Nat(){}
Nat::~Nat(){}
void Nat::handlePacket(Packet pkt){
    std::cout << "nat handling" << std::endl;
    next_->handlePacket(pkt);
}


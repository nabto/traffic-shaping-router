#include "nat.hpp"
#include "packet.hpp"
#include "structures.h"


Nat::Nat(){}
Nat::~Nat(){}
void Nat::handlePacket(Packet pkt){
#ifdef TRACE_LOG
    std::cout << "Applying NAT" << std::endl;
#endif
    next_->handlePacket(pkt);
}


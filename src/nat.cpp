#include "nat.hpp"
#include "packet.hpp"


Nat::Nat(){}
Nat::~Nat(){}
void Nat::handlePacket(Packet pkt) {
    next_->handlePacket(pkt);
}


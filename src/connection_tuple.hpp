#pragma once

#include <tuple>
#include "packet.hpp"


// ConnectionTuple contains connection information for the Nat filter
class ConnectionTuple
{
 public:
    ConnectionTuple(PacketPtr pkt) {
        srcIp_ = pkt->getSourceIP();
        dstIp_ = pkt->getDestinationIP();
        sport_ = pkt->getSourcePort();
        dport_ = pkt->getDestinationPort();
        proto_ = pkt->getProtocol();
    }
    ConnectionTuple(uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, uint8_t proto) {
        srcIp_ = srcIp;
        dstIp_ = dstIp;
        sport_ = srcPort;
        dport_ = dstPort;
        proto_ = proto;
    }
    ConnectionTuple() {}
    void setSrcIP(const uint32_t src){srcIp_ = src;}
    void setDstIP(const uint32_t dst){dstIp_ = dst;}
    void setSport(const uint16_t sport){sport_ = sport;}
    void setDport(const uint16_t dport){dport_ = dport;}

    uint32_t getSrcIP(){return srcIp_;}
    uint32_t getDstIP(){return dstIp_;}
    uint16_t getSport(){return sport_;}
    uint16_t getDport(){return dport_;}
    uint8_t getProto(){return proto_;}

    bool operator==(const ConnectionTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) == std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    bool operator!=(const ConnectionTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) != std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    bool operator<(const ConnectionTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) < std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    bool operator>(const ConnectionTuple& ct) const {
        return (*this<ct);
    }
    bool operator<=(const ConnectionTuple& ct) const {
        return !(*this>ct);
    }
    bool operator>=(const ConnectionTuple& ct) const {
        return !(*this<ct);
    }
    friend std::ostream& operator<<(std::ostream& os, const ConnectionTuple& ct);
 private:
    uint32_t srcIp_;
    uint32_t dstIp_;
    uint16_t sport_;
    uint16_t dport_;
    uint8_t proto_;

};

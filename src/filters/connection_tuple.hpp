#pragma once

#include <tuple>
#include <packet.hpp>


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

    uint32_t getSrcIP() const {return srcIp_;}
    uint32_t getDstIP() const {return dstIp_;}
    uint16_t getSport() const {return sport_;}
    uint16_t getDport() const {return dport_;}
    uint8_t getProto() const {return proto_;}

    bool isEqual(const uint32_t srcIp, const uint32_t dstIp, const uint16_t srcPort, const uint16_t dstPort, const uint8_t proto) const {
        uint32_t msa = srcIp == 0 ? 0 : srcIp_;
        uint32_t mda = dstIp == 0 ? 0 : dstIp_;
        uint16_t msp = srcPort == 0 ? 0 : sport_;
        uint16_t mdp = dstPort == 0 ? 0 : dport_;
        return (std::tie(msa, mda, msp, mdp, proto_) == std::tie(srcIp, dstIp, srcPort, dstPort, proto));
    }

    bool operator==(const ConnectionTuple& ct) const {
        // uint32_t sa1 = ct.srcIp_ == 0 ? 0 : srcIp_;
        // uint32_t sa2 = srcIp_ == 0 ? 0 : ct.srcIp_;
        // uint32_t da1 = ct.dstIp_ == 0 ? 0 : dstIp_;
        // uint32_t da2 = dstIp_ == 0 ? 0 : ct.dstIp_;
        // uint16_t sp1 = ct.sport_ == 0 ? 0 : sport_;
        // uint16_t sp2 = sport_ == 0 ? 0 : ct.sport_;
        // uint16_t dp1 = ct.dport_ == 0 ? 0 : dport_;
        // uint16_t dp2 = dport_ == 0 ? 0 : ct.dport_;
        // return (std::tie(sa1, da1, sp1, dp1, proto_) == std::tie(sa2, da2, sp2, dp2, ct.proto_));
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) == std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    bool operator<(const ConnectionTuple& ct) const {
        // uint32_t sa1 = ct.srcIp_ == 0 ? 0 : srcIp_;
        // uint32_t sa2 = srcIp_ == 0 ? 0 : ct.srcIp_;
        // uint32_t da1 = ct.dstIp_ == 0 ? 0 : dstIp_;
        // uint32_t da2 = dstIp_ == 0 ? 0 : ct.dstIp_;
        // uint16_t sp1 = ct.sport_ == 0 ? 0 : sport_;
        // uint16_t sp2 = sport_ == 0 ? 0 : ct.sport_;
        // uint16_t dp1 = ct.dport_ == 0 ? 0 : dport_;
        // uint16_t dp2 = dport_ == 0 ? 0 : ct.dport_;
        // return (std::tie(sa1, da1, sp1, dp1, proto_) < std::tie(sa2, da2, sp2, dp2, ct.proto_));
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) < std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    bool operator!=(const ConnectionTuple& ct) const {
        return !(*this==ct);
//        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) != std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
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

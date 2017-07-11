#pragma once

#include <tuple>
#include <packet.hpp>


class ConnectionTuple{
 public:
    ConnectionTuple(PacketPtr pkt){
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
    ConnectionTuple(){}
    virtual void setSrcIP(const uint32_t src){srcIp_ = src;}
    virtual void setDstIP(const uint32_t dst){dstIp_ = dst;}
    virtual void setSport(const uint16_t sport){sport_ = sport;}
    virtual void setDport(const uint16_t dport){dport_ = dport;}

    virtual uint32_t getSrcIP() const {return srcIp_;}
    virtual uint32_t getDstIP() const {return dstIp_;}
    virtual uint16_t getSport() const {return sport_;}
    virtual uint16_t getDport() const {return dport_;}
    virtual uint8_t getProto() const {return proto_;}
    friend std::ostream& operator<<(std::ostream& os, const ConnectionTuple& ct);

    virtual bool operator==(const ConnectionTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) == std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    virtual bool operator<(const ConnectionTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, sport_, dport_, proto_) < std::tie(ct.srcIp_, ct.dstIp_, ct.sport_, ct.dport_, ct.proto_));
    }
    virtual bool operator!=(const ConnectionTuple& ct) const {
        return !(*this==ct);
    }
    virtual bool operator>(const ConnectionTuple& ct) const {
        return (*this<ct);
    }
    virtual bool operator<=(const ConnectionTuple& ct) const {
        return !(*this>ct);
    }
    virtual bool operator>=(const ConnectionTuple& ct) const {
        return !(*this<ct);
    }
 protected:
    uint32_t srcIp_;
    uint32_t dstIp_;
    uint16_t sport_;
    uint16_t dport_;
    uint8_t proto_;
};


class SymNatIntTuple : public ConnectionTuple
{
 public:
    SymNatIntTuple(PacketPtr pkt)
        : ConnectionTuple(pkt) { }
    SymNatIntTuple(uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, uint8_t proto)
        : ConnectionTuple(srcIp, dstIp, srcPort, dstPort, proto) { }
    SymNatIntTuple(ConnectionTuple bt)
        : ConnectionTuple(bt.getSrcIP(), bt.getDstIP(), bt.getSport(), bt.getDport(), bt.getProto()) { }

    bool operator==(const SymNatIntTuple& ct) const {
        return (std::tie(srcIp_, sport_, proto_) == std::tie(ct.srcIp_, ct.sport_, ct.proto_));
    }
    bool operator<(const SymNatIntTuple& ct) const {
        return (std::tie(srcIp_, sport_, proto_) < std::tie(ct.srcIp_, ct.sport_, ct.proto_));
    }
};


class FullconeExtTuple : public ConnectionTuple
{
 public:
    FullconeExtTuple(PacketPtr pkt)
        : ConnectionTuple(pkt)
        { }
    FullconeExtTuple(uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, uint8_t proto)
        : ConnectionTuple(srcIp, dstIp, srcPort, dstPort, proto)
        { }
    FullconeExtTuple(ConnectionTuple bt)
        : ConnectionTuple(bt.getSrcIP(), bt.getDstIP(), bt.getSport(), bt.getDport(), bt.getProto()) { }

    bool operator==(const FullconeExtTuple& ct) const {
        return (std::tie(dstIp_, dport_, proto_) == std::tie(ct.dstIp_, ct.dport_, ct.proto_));
    }
    bool operator<(const FullconeExtTuple& ct) const {
        return (std::tie(dstIp_, dport_, proto_) < std::tie(ct.dstIp_, ct.dport_, ct.proto_));
    }
};


class AddrrExtTuple : public ConnectionTuple
{
 public:
    AddrrExtTuple(PacketPtr pkt)
        : ConnectionTuple(pkt)
        { }
    AddrrExtTuple(uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, uint8_t proto)
        : ConnectionTuple(srcIp, dstIp, srcPort, dstPort, proto)
        { }
    AddrrExtTuple(ConnectionTuple bt)
        : ConnectionTuple(bt.getSrcIP(), bt.getDstIP(), bt.getSport(), bt.getDport(), bt.getProto()) { }

    bool operator==(const AddrrExtTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, dport_, proto_) == std::tie(ct.srcIp_, ct.dstIp_, ct.dport_, ct.proto_));
    }
    bool operator<(const AddrrExtTuple& ct) const {
        return (std::tie(srcIp_, dstIp_, dport_, proto_) < std::tie(ct.srcIp_, ct.dstIp_, ct.dport_, ct.proto_));
    }
};


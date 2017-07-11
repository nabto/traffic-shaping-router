#include "packet.hpp"
#include "filter.hpp"
#include <filters/shaper.hpp>
#include <filters/nat.hpp>
#include <filters/burst_filter.hpp>
#include <filters/tbf_filter.hpp>
#include <filters/output.hpp>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE router_filter_test

#include <boost/test/unit_test.hpp>

std::string intIp = "10.0.0.1";
std::string extIp = "15.0.0.1";
std::string remIp1 = "20.0.0.1";
std::string remIp2 = "30.0.0.1";
int16_t intPort = 5000;
int16_t remPort1 = 5300;
int16_t remPort2 = 5400;

class TestFilter : public Filter, public std::enable_shared_from_this<TestFilter>
{
 public:
    TestFilter() {
        pktHandled_ = false;
    }
    ~TestFilter() {}
    void handlePacket(PacketPtr pkt) {
        pkt_ = pkt;
        pktHandled_ = true;
    }
    bool handledPacket() {return pktHandled_;}
    PacketPtr getHandledPacket(){ return pkt_;}
    void reset() { pkt_ = nullptr; pktHandled_ = false;}

 private:
    PacketPtr pkt_;
    bool pktHandled_;
};



//BOOST_AUTO_TEST_SUITE(basic)

BOOST_AUTO_TEST_CASE(TestNatFilterSymNat)
{
    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setNatType("symnat");

    // SENDING INITIAL PACKET TO OPEN A PORT IN NAT
    PacketPtr pkt = std::make_shared<Packet>();
    pkt->setSourceIP(inet_network(intIp.c_str()));
    pkt->setDestinationIP(inet_network(remIp1.c_str()));
    pkt->setSourcePort(intPort);
    pkt->setDestinationPort(remPort1);

    nat->handlePacket(pkt);
    // internal packet should always be forwarded with the external IP, intPort should not change in first connection
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == inet_network(extIp.c_str()));
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);

    // Sending packet back to check port is open
    PacketPtr pkt2 = std::make_shared<Packet>();
    pkt2->setSourceIP(inet_network(remIp1.c_str()));
    pkt2->setDestinationIP(tst->getHandledPacket()->getSourceIP());
    pkt2->setSourcePort(remPort1);
    pkt2->setDestinationPort(tst->getHandledPacket()->getSourcePort());

    tst->reset();
    nat->handlePacket(pkt2);

    // Internal host should see original IPs and Port numbers
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == inet_network(remIp1.c_str()));
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == inet_network(intIp.c_str()));
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    tst->reset();

    // Sending packet from internal host on the same port with different destination
    pkt->setSourceIP(inet_network(intIp.c_str()));
    pkt->setDestinationIP(inet_network(remIp1.c_str()));
    pkt->setSourcePort(intPort);
    pkt->setDestinationPort(remPort2);

    nat->handlePacket(pkt);
    
    // internal packet should always be forwarded with the external IP.
    // Second time the intPort is used, the port number should change through the NAT
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == inet_network(extIp.c_str()));
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() != intPort);

}

//BOOST_AUTO_TEST_SUITE_END()

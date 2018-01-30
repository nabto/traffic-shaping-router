#include "parent_packet.hpp"
#include "filter.hpp"
#include <filters/shaper.hpp>
#include <filters/nat.hpp>
#include <filters/burst_filter.hpp>
#include <filters/tbf_filter.hpp>
#include <filters/output.hpp>
#include <filters/dynamic_delay.hpp>

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
    void handlePacket(std::shared_ptr<ParentPacket> pkt) {
        pkt_ = std::dynamic_pointer_cast<Packet>(pkt);
        pktHandled_ = true;
    }
    bool handledPacket() {return pktHandled_;}
    PacketPtr getHandledPacket(){ return pkt_;}
    void reset() { pkt_ = nullptr; pktHandled_ = false;}

 private:
    PacketPtr pkt_;
    bool pktHandled_;
};


BOOST_AUTO_TEST_CASE(TestDynamicDelayFilter)
{
    std::shared_ptr<DynamicDelay> dd;
    dd = std::make_shared<DynamicDelay>();
    std::vector<int> tmpQ = dd->setDelays({0,8,5},4);
    std::cout << "tmpQ:\n[";
    for (auto i : tmpQ){
        std::cout << i;
    }
    std::cout << "]" << std::endl;
    std::vector<int> v = {0,2,4,6,8,7,6,5,5,3,2,1};
    BOOST_REQUIRE((tmpQ == v));
}    


//BOOST_AUTO_TEST_SUITE(basic)

BOOST_AUTO_TEST_CASE(TestNatFilterSymNat)
{
    /*************************************
     * test packets for sym nat
     * packet 1:
     * iaddr -> raddr1 | eaddr -> raddr1
     * iport -> rport1 | iport -> rport1
     * 
     * packet 2:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport1 -> iport  | rport1 -> iport
     * 
     * packet 3:
     * raddr2 -> eaddr  |    \/
     * rport1 -> iport  |    /\
     * 
     * packet 4:
     * raddr1 -> eaddr  |    \/
     * rport2 -> iport  |    /\
     * 
     * packet 5:
     * raddr2 -> eaddr  |    \/
     * rport2 -> iport  |    /\
     * 
     * packet 6:
     * iaddr  -> raddr1 | eaddr  -> raddr1
     * iport  -> rport2 | eport2 -> rport2
     * 
     *************************************/
    uint32_t intIp_ = inet_network(intIp.c_str());
    uint32_t extIp_ = inet_network(extIp.c_str());
    uint32_t remIp1_ = inet_network(remIp1.c_str());
    uint32_t remIp2_ = inet_network(remIp2.c_str());

    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setNatType("symnat");
    nat->init();

    // Packet 1
    PacketPtr pkt = std::make_shared<Packet>(intIp_, remIp1_, intPort, remPort1);
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 3
    pkt.reset(new Packet(remIp2_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 4
    pkt.reset(new Packet(remIp1_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 5
    pkt.reset(new Packet(remIp2_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 6
    pkt.reset(new Packet(intIp_, remIp1_, intPort, remPort2));
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() != intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort2);

    tst->reset();
    
}

BOOST_AUTO_TEST_CASE(TestNatFilterFullconeNat)
{
    /*************************************
     * test packets for fullcone nat
     * packet 1:
     * iaddr -> raddr1 | eaddr -> raddr1
     * iport -> rport1 | iport -> rport1
     * 
     * packet 2:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport1 -> iport  | rport1 -> iport
     * 
     * packet 3:
     * raddr2 -> eaddr  | raddr2 -> iaddr
     * rport1 -> iport  | rport1 -> iport
     * 
     * packet 4:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport2 -> iport  | rport2 -> iport
     * 
     * packet 5:
     * raddr2 -> eaddr  | raddr2 -> iaddr
     * rport2 -> iport  | rport2 -> iport
     * 
     * packet 6:
     * iaddr  -> raddr2 | eaddr  -> raddr2
     * iport  -> rport2 | iport  -> rport2
     * 
     *************************************/
    uint32_t intIp_ = inet_network(intIp.c_str());
    uint32_t extIp_ = inet_network(extIp.c_str());
    uint32_t remIp1_ = inet_network(remIp1.c_str());
    uint32_t remIp2_ = inet_network(remIp2.c_str());

    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setNatType("fullcone");
    nat->init();

    // Packet 1
    PacketPtr pkt = std::make_shared<Packet>(intIp_, remIp1_, intPort, remPort1);
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 3
    pkt.reset(new Packet(remIp2_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp2_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 4
    pkt.reset(new Packet(remIp1_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort2);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 5
    pkt.reset(new Packet(remIp2_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp2_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort2);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 6
    pkt.reset(new Packet(intIp_, remIp2_, intPort, remPort2));
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp2_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort2);

    tst->reset();
}

BOOST_AUTO_TEST_CASE(TestNatFilterPortrNat)
{
    /*************************************
     * test packets for Port restricted nat
     * packet 1:
     * iaddr -> raddr1 | eaddr -> raddr1
     * iport -> rport1 | iport -> rport1
     * 
     * packet 2:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport1 -> iport  | rport1 -> iport
     * 
     * packet 3:
     * raddr2 -> eaddr  |    \/
     * rport1 -> iport  |    /\
     * 
     * packet 4:
     * raddr1 -> eaddr  |    \/
     * rport2 -> iport  |    /\
     * 
     * packet 5:
     * raddr2 -> eaddr  |    \/
     * rport2 -> iport  |    /\
     * 
     * packet 6:
     * iaddr  -> raddr1 | eaddr  -> raddr1
     * iport  -> rport2 | iport -> rport2
     * 
     *************************************/
    uint32_t intIp_ = inet_network(intIp.c_str());
    uint32_t extIp_ = inet_network(extIp.c_str());
    uint32_t remIp1_ = inet_network(remIp1.c_str());
    uint32_t remIp2_ = inet_network(remIp2.c_str());

    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setNatType("portr");
    nat->init();

    // Packet 1
    PacketPtr pkt = std::make_shared<Packet>(intIp_, remIp1_, intPort, remPort1);
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 3
    pkt.reset(new Packet(remIp2_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 4
    pkt.reset(new Packet(remIp1_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 5
    pkt.reset(new Packet(remIp2_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 6
    pkt.reset(new Packet(intIp_, remIp1_, intPort, remPort2));
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort2);

    tst->reset();
    
}

BOOST_AUTO_TEST_CASE(TestNatFilterAddrNat)
{
    /*************************************
     * test packets for address restricted nat
     * packet 1:
     * iaddr -> raddr1 | eaddr -> raddr1
     * iport -> rport1 | iport -> rport1
     * 
     * packet 2:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport1 -> iport  | rport1 -> iport
     * 
     * packet 3:
     * raddr2 -> eaddr  |     \/
     * rport1 -> iport  |     /\
     * 
     * packet 4:
     * raddr1 -> eaddr  | raddr1 -> iaddr
     * rport2 -> iport  | rport2 -> iport
     * 
     * packet 5:
     * raddr2 -> eaddr  |     \/
     * rport2 -> iport  |     /\
     * 
     * packet 6:
     * iaddr  -> raddr2 | eaddr  -> raddr2
     * iport  -> rport2 | iport  -> rport2
     * 
     *************************************/
    uint32_t intIp_ = inet_network(intIp.c_str());
    uint32_t extIp_ = inet_network(extIp.c_str());
    uint32_t remIp1_ = inet_network(remIp1.c_str());
    uint32_t remIp2_ = inet_network(remIp2.c_str());

    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setNatType("addrr");
    nat->init();

    // Packet 1
    PacketPtr pkt = std::make_shared<Packet>(intIp_, remIp1_, intPort, remPort1);
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 3
    pkt.reset(new Packet(remIp2_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 4
    pkt.reset(new Packet(remIp1_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort2);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    // Packet 5
    pkt.reset(new Packet(remIp2_,extIp_,remPort2,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    // Packet 6
    pkt.reset(new Packet(intIp_, remIp2_, intPort, remPort2));
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp2_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort2);

    tst->reset();
}

BOOST_AUTO_TEST_CASE(testNatConnectionTimeout){
    uint32_t intIp_ = inet_network(intIp.c_str());
    uint32_t extIp_ = inet_network(extIp.c_str());
    uint32_t remIp1_ = inet_network(remIp1.c_str());

    std::shared_ptr<TestFilter> tst;
    std::shared_ptr<Nat> nat;
    nat = std::make_shared<Nat>();
    tst = std::make_shared<TestFilter>();
    nat->setNext(tst);
    nat->setIPs(extIp, intIp);
    nat->setTimeout(1);
    nat->setNatType("fullcone");
    nat->init();


    PacketPtr pkt = std::make_shared<Packet>(intIp_, remIp1_, intPort, remPort1);
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);

    sleep(2);
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(!tst->handledPacket());

    pkt.reset(new Packet(intIp_, remIp1_, intPort, remPort1));
    pkt->setIngoing(false);
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == extIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == intPort);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == remPort1);

    // Packet 2
    pkt.reset(new Packet(remIp1_,extIp_,remPort1,intPort));
    pkt->setIngoing(true);
    tst->reset();
    nat->handlePacket(pkt);
    BOOST_REQUIRE(tst->handledPacket());
    BOOST_REQUIRE(tst->getHandledPacket()->getSourceIP() == remIp1_);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationIP() == intIp_);
    BOOST_REQUIRE(tst->getHandledPacket()->getSourcePort() == remPort1);
    BOOST_REQUIRE(tst->getHandledPacket()->getDestinationPort() == intPort);


}

//BOOST_AUTO_TEST_SUITE_END()

#ifndef OUTPUT6LIBNET_HPP
#define OUTPUT6LIBNET_HPP

#include <filter.hpp>
#include <packet6.hpp>
#include <tp_service.hpp>
#include <async_queue.hpp>
#include <libnet.h>

// The output filter is the last filter, which sends the packet on the network
class Output6Libnet : public Filter, public std::enable_shared_from_this<Output6Libnet>
{
 public:
    Output6Libnet();
    ~Output6Libnet();
    bool init();
    // handlePacket pushes pkt to an AsyncQueue and returns
    void handlePacket(std::shared_ptr<ParentPacket> pkt);
    // Pophandler handles packets as they are poped from the queue
    void popHandler(const std::error_code& ec, const Packet6Ptr pkt);

 private:
    AsyncQueue<Packet6Ptr > queue_;
    libnet_t  *l_;
    char errbuf_[LIBNET_ERRBUF_SIZE];
    void dumpMem(uint8_t* p, int len);
};


#endif //OUTPUT6_HPP

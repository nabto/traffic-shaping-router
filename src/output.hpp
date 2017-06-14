#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "filter.hpp"
#include "packet.hpp"
#include "tpService.hpp"
#include "async_queue.hpp"
#include <libnet.h>

class Output : public Filter, public std::enable_shared_from_this<Output>
{
 public:
    Output();
    ~Output();
    void handlePacket(PacketPtr pkt);
    void init();
    void popHandler(const boost::system::error_code& ec, const PacketPtr pkt);

 private:
    AsyncQueue<PacketPtr> queue_;
    libnet_t  *l_;
    char errbuf_[LIBNET_ERRBUF_SIZE];
    void dumpMem(uint8_t* p, int len);
};


#endif //OUTPUT_HPP
#ifndef SHAPER_HPP
#define SHAPER_HPP

#include router.hpp

class shaper
{
 public:
    shaper();
    ~shaper();
    ROUTER_STATUS handle_packet();
};

#endif // SHAPER_HPP

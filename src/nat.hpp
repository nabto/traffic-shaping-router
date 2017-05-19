#ifndef NAT_HPP
#define NAT_HPP

#include router.hpp

class nat
{
 public:
    nat();
    ~nat();
    NAT_STATUS handle_packet();
};

#endif // NAT_HPP

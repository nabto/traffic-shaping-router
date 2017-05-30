#include "router.hpp"
#include <unistd.h>


int main (int argc, char* argv[])
{
    RouterPtr rt = Router::getInstance();
    rt->setDelay(100);
    rt->setLoss(0.1);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
    sleep(2);
}

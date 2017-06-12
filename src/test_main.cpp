#include "router.hpp"
#include <unistd.h>


int main (int argc, char* argv[])
{
    RouterPtr rt = Router::getInstance();
    rt->setDelay(100);
    rt->setLoss(0.1);
    rt->setExtIf("eth0");
    rt->setExtIp("192.72.0.2");
    rt->setIntIf("eth1");
    rt->setIntIf("10.0.2.2");
    rt->init();
    rt->newPacket(NULL,NULL,NULL,NULL);
    usleep(2000);
    rt->newPacket(NULL,NULL,NULL,NULL);
/*    usleep(2000);
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
    rt->newPacket(NULL,NULL,NULL,NULL);*/
    sleep(1);
}

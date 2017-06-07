#include "router.hpp"
#include <stdlib.h>
#include <iostream>

int main (int argc, char* argv[])
{
    if(argc != 4){
        std::cout << "Wrong amount of input arguments" << std::endl;
        std::cout << "usage: ./router delay loss outsideInterface" << std::endl;
        return 0;
    }
    int del = atoi(argv[1]);
    float los = atof(argv[2]);
    std::cout << "Router started with delay: " << del << ", loss: " << los << ", and IF " << argv[3] << std::endl;
    RouterPtr rt = Router::getInstance();
    rt->setDelay(del);
    rt->setLoss(los);
    rt->setOutIf(std::string(argv[3]));
    rt->init();
    rt->execute();

}

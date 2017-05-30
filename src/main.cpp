#include "router.hpp"
#include <stdlib.h>
#include <iostream>

int main (int argc, char* argv[])
{
    if(argc != 3){
        std::cout << "Wrong amount of input arguments" << std::endl;
        std::cout << "usage: ./router delay loss" << std::endl;
        return 0;
    }
    int del = atoi(argv[1]);
    float los = atof(argv[2]);
    std::cout << "Router started with delay: " << del << " and loss: " << los << std::endl;
    RouterPtr rt = Router::getInstance();
    rt->setDelay(del);
    rt->setLoss(los);
    rt->init();
    rt->execute();

}

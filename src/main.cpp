#include "router.hpp"
#include <iostream>

int main (int argc, char* argv[])
{
    std::cout << "Router started" << std::endl;
    RouterPtr rt = Router::getInstance();
    rt->init();
    rt->execute();

}

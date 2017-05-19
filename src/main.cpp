#include "router.hpp"


int main (int argc, char* argv[])
{
    RouterPtr rt = Router::getInstance();
    rt->init();
    rt->execute();

}

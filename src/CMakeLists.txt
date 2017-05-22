cmake_minimum_required (VERSION 2.8)
project (Router)

add_library(test_router_src test_router.cpp)
add_library(packet_src packet.cpp)
add_library(nat_src nat.cpp)
add_library(router_src router.cpp)
add_library(shaper_src shaper.cpp)
include_directories("/usr/include/boost")
set(CFLAGS "-fPIC -Wall -Werror -g")

add_definitions(${CFLAGS})



add_executable(test_router test_main.cpp)
add_executable(router main.cpp)

#find_library(libnet NAMES net libnet PATHS /usr/lib/x86_64-linux-gnu)
#include_directories(<wherever you built it to>/include)
#target_link_libraries(router libnet)

#target_link_libraries(Router libnet)

#find_library(libnet NAMES libnet.so PATHS /usr/lib/x86_64-linux-gnu/)
#target_link_libraries(router LIBNET_LIB)
link_directories("/usr/lib/x86_64-linux-gnu")
target_link_libraries(router router_src packet_src nat_src shaper_src netfilter_queue nfnetlink boost_system boost_thread libnet.so)
target_link_libraries(test_router test_router_src packet_src nat_src shaper_src netfilter_queue nfnetlink boost_system boost_thread libnet.so)

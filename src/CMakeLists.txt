cmake_minimum_required (VERSION 2.8)
project (Router)

set(target_src
  tp_service.cpp
  packet.cpp
  )

set(target_hdr
  filter.hpp
  packet.hpp
  )
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory("filters")

set(router_src
  ${target_hdr}
  ${target_src}
  router.cpp
  main.cpp
  )

set(test_router_src
  ${target_hdr}
  ${target_src}
  test_router.cpp
  test_main.cpp
  )

include_directories("/usr/include/boost")
set(CFLAGS "-fPIC -Wall -Werror -g")
add_definitions(-std=c++11)
add_definitions(${CFLAGS})

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("Enabling trace log")
  add_definitions(-DTRACE_LOG=1)
endif ()

# add_executable(test_router ${test_router_src})
add_executable(router ${router_src})

link_directories("/usr/lib/x86_64-linux-gnu")
target_link_libraries(router
  netfilter_queue
  nfnetlink
  boost_system
  boost_thread
  boost_program_options
  boost_date_time
  libnet.so)
# target_link_libraries(test_router
#   netfilter_queue
#   nfnetlink
#   boost_system
#   boost_thread
#   boost_program_options
#   boost_date_time
#   libnet.so)

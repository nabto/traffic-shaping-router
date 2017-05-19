CXX ?= g++

# path #
SRC_PATH = src
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin

# extensions #
SRC_EXT = cpp

# code lists #
# Find all source files in the source directory, sorted by
CC = gcc
CPP = g++
CFLAGS =  -fPIC -Wall -Werror -g -static-libstdc++
LDFLAGS=  -static-libstdc++ -lnetfilter_queue -lnfnetlink -I/usr/include/boost/ -lboost_system -lboost_thread
INCLUDES = 

OBJS = main.o packet.o router.o /usr/lib/x86_64-linux-gnu/libnet.so

.cpp.o:
	$(CPP) -c $(INCLUDES) $(CFLAGS) $<

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $<

all: $(OBJS)
	$(CPP) -o router $(OBJS) $(LDFLAGS)

#depend: *.cpp
#	rm -f .depend
#	$(CPP) -M $(INCLUDES) $(CFLAGS) *.cpp > .depend

clean:
	rm -f *.o core .depend*

# Include the dependency information from make depend
ifeq (.depend,$(wildcard .depend))
include .depend
endif


#  cp router ../../nabto/test/dockertest/containers/nabto_router_node/

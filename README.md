# traffic shaping router

The Traffic shaping router is a linux router designed to emulate different network behaviour for the purpose of testing software. The router is based on iptables, which takes all packets directly from the network interfaces, and sends them through a series of chains which can change packets and make decisions on where to send packets. Iptables uses 5 main chains, PREROUTING applies DNAT as the first chain when a packet is received on an interface. A routing decision is then made whether the packet is destined for the loacl host or is to be forwarded to another interface. Packets for the local host is send to the INPUT chain whereas packets for other hosts is send to the FORWARD chain. These chains can then apply firewall rules where unwanted packets can be dropped, and wanted packets can be passed on. Packets originating from the local host is send to the OUTPUT chain which can perform similar firewall actions. When a packet leaves the OUTPUT or FORWARD chains, they are sent to the POSTROUTING chain which applies SNAT before sending it out on the network interface.

The router works by forwarding all packets coming into the INPUT and FORWARD chains to the user space by using nf queue. This means the firewall rules applied to these two chains should put packets into the nf queue (the router uses queue number 0). The commands for applying these rules is shown here:

```
iptables -A FORWARD -j NFQUEUE --queue-num 0
iptables -A INPUT -j NFQUEUE --queue-num 0
```
If the router host should be capable of using the network for connections not going through the nf queue, these packets should be accepted from the INPUT chain before reaching the nf queue. Below it is shown how to accept connections from the internal network, and to accept connections initiated by the router host itself. The router will send packets on the OUTPUT chain. For this traffic not to be considered a connection initiated by the router host, all traffic going through the router must have connection tracking turned off:

```
iptables -A INPUT ! -i eth0 -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -t raw -A OUTPUT ! -s 10.0.2.2/24 -j NOTRACK
```

## Building the Router
Follow the steps below to build the router. Building the router requires git, cmake, boost library, libnet library, and iptables.
```
git clone https://github.com/nabto/traffic-shaping-router.git
cd traffic-shaping-router
mkdir build
cd build
cmake ..
make -j
```
Running ./router -h yields:
```
Allowed options:
  -h [ --help ]         Print usage message
  --ext_ip arg          The ip of the external interface of the router
  --int_ip arg          The ip of the internal interface of the router
  -d [ --del ] arg      The network delay in ms to be imposed
  -l [ --loss ] arg     The packet loss probability
  -e [ --ext_nat ] arg  External port numbers for port forwarding
  -i [ --int_nat ] arg  Internal port numbers for port forwarding, if defined 
                        must have same length as ext_nat, if not defined 
                        ext_nat will be reused for internal ports
```
ext_ip and int_ip is the external and internal IP address of the router, these two fields are required to run the router.
del sets the static delay imposed on all packets, and loss sets the packet loss probability. If these fields are not set, they will default to 0.

ext_nat and int_nat can be used to set port forwarding on the router. By setting these, a new connection comming to the external IP address on port 'ext_nat' will be forwarded to the internal IP address on port 'int_nat'. These fields are defined as arrays so multiple ports can be forwarded (e.g. --ext_nat 4030 2040 5030 --int_nat 1021 6600 2010). The lengths of ext_nat and int_nat must be equal.

## Overall implementation design
The main function creates an instance of the Router class, configures it, and calls the Routers ```execute()``` function. The Router class then starts receiving packets from the nf queue. When a packet is received, it is saved in memory, and dropped from the nf queue. The packet is then send through a series of filters which is classes extending the Filter class. Each filter can alter the packet, and choose wether or not to send it to the next filter in the series. If all filters choose to forward the packet, it ends in the Output filter which will send the packet to the network through the iptables OUTPUT chain.  Currently, 3 filters exists not counting the Output filter:

  * The Loss filter randomly drops packets based on a given probability.
  * The Nat filter implements port restricted nat. It also supports port forwarding.
  * The StaticDelay filter delays all packets with a static delay.

These filters are described in detail below.


## Filters
### Loss
The loss filter will simply draw a random float between 0 and 1, and compares it to the provided packet loss probability. If the drawn number is below the provided value, the packet is dropped, otherwise it is forwarded to the next filter.
### Nat
The Nat filter implements port restricted nat. Currently only a single internal IP is supported, which means no port clashes can occur. The nat will, therefore, never translate port numbers, (i.e. if an internal ip sends with source port X, the router will always forward the packet with source port X). This means the nat only translates the IP addresses, with the exception of port forwarding rules, where it is possible to specify an internal port number.

For ICMP request/reply packets, the Nat filter will use the ICMP ID as port number for connection identification. Connections are compared on source IP, Destination IP, Source Port, Destination Port, and Protocol. This ensures that connections where ICMP IDs are clashing with port numbers can be distinguished.

### StaticDelay
The StaticDelay filter puts incoming packets into a queue from where they are poped asynchroniously. When a packet is popped from the queue, it is not forwarded to the next filter until they have been in the router for the amount milliseconds defined by the del command line argument. This filter promises no packet reordering. 

## Testing in Docker
The router is developed for testing in a docker container. The test folder on the repository contains an example of using the router in a docker-compose setup. In the test folder is three subfolders: compose, ping_node, and router_node.
The router_node folder contains the Dockerfile building a docker container for the router software which is connected to the Internet, and to an internal network. The script router.sh is executed in the container when the docker-compose starts it. This script starts tcpdump to log all network packets, it then sets up the needed iptables rules, and finally starts the router.
The ping_node folder contains the Dockerfile building a docker container for a node on the internal network behind the router. In this node, the ping.sh script can be executed which again logs traffic using tcpdump. It then pings google.com to show delay and loss are working properly, but also that ICMP and UDP works. It will then download the index.html file at www.google.com to show TCP works.
The compose folder contains the docker-compose.yml file which configures the two nodes as well as the needed networks. The tester.sh script can be used to run the example with the command ```./tester.sh test```. Each step performed by this command can also be done individually. ```./tester.sh create``` will copy the compiled router software from the build folder created earlier, both Docker containers are then build, and finally they are started. ```./tester.sh run_test``` will execute the ping.sh script in the ping_node running the test. ```./tester.sh clean_up``` will save logs from the containers, stop the containers and remove them. Finally,  will run all the above commands in sequence.

Port forwarding is not tested with the test performed by this example as this would require a third container. To test it manually, the example makes a nat rule which opens port 5201. Running ```./tester.sh create``` starts the containers, then running ```docker-compose exec pinger bash``` and then run ```iperf3 -s``` on the ping_node. Then from another terminal window, run ```iperf3 -c X.X.X.X``` where X.X.X.X is the external IP address of the router node.
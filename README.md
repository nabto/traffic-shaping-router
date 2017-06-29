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

## Overall implementation design
The main function creates an instance of the Router class, configures it, and calls the Routers ```execute()``` function. The Router class then starts receiving packets from the nf queue. When a packet is received, it is saved in memory, and dropped from the nf queue. The packet is then send through a series of filters which is classes extending the Filter class. Each filter can alter the packet, and choose wether or not to send it to the next filter in the series. If all filters choose to forward the packet, it ends in the Output filter which will send the packet to the network through the iptables OUTPUT chain.  Currently, 5 filters exists not counting the Output filter:

  * The Nat filter implements port restricted nat. It also supports port forwarding.
  * The Loss filter randomly drops packets based on a given probability.
  * The StaticDelay filter delays all packets with a static delay.
  * The Burst filter buffers all incoming packets for a given amount of time, and then forwards all simultaniously
  * The TokenBucketFilter is used to impose bandwidth limitations

Running ./router -h yields:
```
Allowed options:
  -h [ --help ]             Print usage message
  --nat-ext-ip arg          The ip of the external interface of the router 
                            (Required)
  --nat-int-ip arg          The ip of the internal interface of the router 
                            (Required with Nat filter)
  -d [ --delay ] arg        The network delay in ms to be imposed
  -l [ --loss ] arg         The packet loss probability
  --tbf-in-rate arg         The rate limit in kbit pr. sec. for the token 
                            bucket filter
  --tbf-in-max-tokens arg   The maximum amount of tokens for the token bucket 
                            filter
  --tbf-in-max-packets arg  The packet queue length of the token bucket filter
  --tbf-in-red-start arg    The packet queue length at which random early drop 
                            kick in
  --tbf-in-red-drop arg     The packet drop probability used for random early 
                            drop
  --tbf-out-rate arg        The rate limit in kbit pr. sec. for the token 
                            bucket filter
  --tbf-out-max-tokens arg  The maximum amount of tokens for the token bucket 
                            filter
  --tbf-out-max-packets arg The packet queue length of the token bucket filter
  --tbf-out-red-start arg   The packet queue length at which random early drop 
                            kick in
  --tbf-out-red-drop arg    The packet drop probability used for random early 
                            drop
  --burst-dur arg           The time in ms when packets are collected for 
                            bursts
  --burst-sleep arg         The time in ms between bursts
  -e [ --nat-ext-port ] arg External port numbers for port forwarding
  -i [ --nat-int-port ] arg Internal port numbers for port forwarding, if 
                            defined must have same length as nat-ext-port, if 
                            not defined nat-ext-port will be reused for 
                            internal ports
  --filters arg             Ordered list of filters to use, valid filters are: 
                            StaticDelay, Loss, Nat, Burst, TokenBucketFilter
```
The filters argument determines which filters to use. If this option is used, only the filters listed will be added to the filter chain, and will be added in the order they are listed. If this option is not used, all filters are added in the order: Loss, Nat, Burst, Delay, TokenBucketFilter. Options for a filter which is not added is ignored silently.

nat-ext-ip and nat-int-ip is the external and internal IP address of the router, these two fields are required to use the Nat filter in the router.

nat-ext-port and nat-int-port can be used to set port forwarding on the router. By setting these, a new connection comming to the external IP address on port 'nat-ext-port' will be forwarded to the internal IP address on port 'nat-int-port'. These fields are defined as arrays so multiple ports can be forwarded (e.g. --nat-ext-port 4030 2040 5030 --nat-int-port 1021 6600 2010). If only nat-ext-port is defined, the listed port numbers will be reused for internal port numbers. If nat-int-port is defined, it must be exactly the same length as nat-ext-port.

The tbf- options exists as both tbf-in- and tbf-out- options, tbf-in- options are applied to packets coming in from the external interface to the internal network. Similarly tbf-out- options are applied to packets coming from the internal network going out of the external interface. For this documentation, only tbf-in- is described but can be interchanged with tbf-out-.

## Filters
### Loss
The loss filter will simply draw a random float between 0 and 1, and compares it to the provided packet loss probability. If the drawn number is below the provided value, the packet is dropped, otherwise it is forwarded to the next filter.
### Nat
The Nat filter implements port restricted nat. Currently only a single internal IP is supported, which means no port clashes can occur. The nat will, therefore, never translate port numbers, (i.e. if an internal ip sends with source port X, the router will always forward the packet with source port X). This means the nat only translates the IP addresses, with the exception of port forwarding rules, where it is possible to specify an internal port number.

For ICMP request/reply packets, the Nat filter will use the ICMP ID as port number for connection identification. Connections are compared on source IP, Destination IP, Source Port, Destination Port, and Protocol. This ensures that connections where ICMP IDs are clashing with port numbers can be distinguished.

### StaticDelay
The StaticDelay filter puts incoming packets into a queue from where they are poped asynchroniously. When a packet is popped from the queue, it is not forwarded to the next filter until they have been in the router for the amount milliseconds defined by the del command line argument. This filter promises no packet reordering. 

### Burst
The burst filter buffers all incoming packets for --burst-dur milliseconds, after which all buffered packets are send simultaniously. The filter then forwards all incoming packets directly for --burst-sleep milliseconds, and then starts over.

### TokenBucketFilter
The TokenBucketFilter is used for bit rate limitations. All incoming packets are put into a queue. The filter adds tokens to its token counter every 4 milliseconds based on the provided --tbf-in-rate up to a maximum of --tbf-in-max-tokens. When the number of tokens exceeds the byte length of the first packet in the packet queue, that packet is forwarded to the next filter. If packets are arriving faster than can be serviced, the queue builds up. When the queue length reaches --tbf-in-red-start Random Early Detection kicks in, and starts dropping incoming packets with the probability --tbf-in-red-drop. When the queue length reaches --tbf-max-packets, all incoming packets are dropped. --tbf-in-red-drop defaults to 0, which disables RED making the queue a simple drop tail queue.

## Testing in Docker
The router is developed for testing in a docker container. The test folder on the repository contains an example of using the router in a docker-compose setup. In the test folder is three subfolders: compose, ping_node, and router_node.
The router_node folder contains the Dockerfile building a docker container for the router software which is connected to the Internet, and to an internal network. The script router.sh is executed in the container when the docker-compose starts it. This script starts tcpdump to log all network packets, it then sets up the needed iptables rules, and finally starts the router.
The ping_node folder contains the Dockerfile building a docker container for a node on the internal network behind the router. In this node, the ping.sh script can be executed which again logs traffic using tcpdump. It then pings google.com to show delay and loss are working properly, but also that ICMP and UDP works. It will then download the index.html file at www.google.com to show TCP works.
The compose folder contains the docker-compose.yml file which configures the two nodes as well as the needed networks. The tester.sh script can be used to run the example with the command ```./tester.sh test```. Each step performed by this command can also be done individually. ```./tester.sh create``` will copy the compiled router software from the build folder created earlier, both Docker containers are then build, and finally they are started. ```./tester.sh run_test``` will execute the ping.sh script in the ping_node running the test. ```./tester.sh clean_up``` will save logs from the containers, stop the containers and remove them. 

Port forwarding is not tested with the test performed by this example as this would require a third container. To test it manually, the example makes a nat rule which opens port 5201. Running ```./tester.sh create``` starts the containers, then running ```docker-compose exec pinger bash``` and then run ```iperf3 -s``` on the ping_node. Then from another terminal window, run ```iperf3 -c X.X.X.X``` where X.X.X.X is the external IP address of the router node.
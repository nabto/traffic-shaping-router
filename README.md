# traffic shaping router

## Motivation
The Traffic shaping router is a linux router designed to emulate different network behaviour for the purpose of testing software. The router is motivated by the limited flexibility and general complexity of tc in the linux kernel. The netEm option in linux tc can impose network constraints on traffic, but is designed for steady state network emulation. Even with the reordering option turned off, varying delays either by reconfiguration or by use of the jitter functionallity causes packet reordering in the emulator. 

Varying delays in network will often be caused by cross-traffic from other users causing longer queues in network equipment, which should not impose packet reordering. This functionallity is not available in netEm. Linux tc is also designed to work on outgoing traffic only, though this can be circumvented by piping traffic, this is a hassle to get working. netEm offers thoughput limitation through the rate option. This option is made as a replacement for the TBF option in tc since using TBF together with netEm required sequential queueing disciplines in tc which, though possible, is hard to get working. 

If other queueing disciplines is to be emulated (like RED or HTB), these would still require sequential queing disciplines in tc. The Traffic Shaping Router aims to simplify network emulation significantly, while increasing the flexibility of the emulation. The Traffic Shaping Router takes each packet and passes it though a simple chain of filters which can alter, delay, or drop packets. This approach greatly simplifies the usage compared to using several queing disciplines in tc. 

The Traffic Shaping Router defines a simple c++ interface for the filters. This means any functionallity not directly covered by existing filters can easily be added by implementing a new filter. As an example of the simplicity of implementing a filter, below is an example of a filter randomly dropping packets with a 1% probability:

```
class Loss : public Filter, public std::enable_shared_from_this<Loss>
{
 public:
    Loss() {srand (static_cast <unsigned> (time(0)));}
    ~Loss() {}
    // Packet handler which drops packets randomly
    void handlePacket(PacketPtr pkt){
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        if (r < 0.01){
            pkt->setVerdict(false);
            return;
        }
        next_->handlePacket(pkt);
    }
};
```

## Basic design
The router is based on iptables, which takes all packets directly from the network interfaces and sends them through a series of chains which can change packets and make decisions on where to send packets. Iptables uses 5 main chains, PREROUTING applies DNAT as the first chain when a packet is received on an interface. A routing decision is then made whether the packet is destined for the local host or is to be forwarded to another interface. Packets for the local host is send to the INPUT chain whereas packets for other hosts are sent to the FORWARD chain. These chains can then apply firewall rules where unwanted packets can be dropped, and wanted packets can be passed on. Packets originating from the local host is send to the OUTPUT chain which can perform similar firewall actions. When a packet leaves the OUTPUT or FORWARD chains, they are sent to the POSTROUTING chain which applies SNAT before sending it out on the network interface.

The router works by redirecting traffic coming into iptable chains to the user space by using the nf queue. This means the firewall rules applied to the chains should put packets into the nf queue (the router uses queue number 0). The command for applying such a rule to the INPUT chain is shown here:

```
iptables -A INPUT -j NFQUEUE --queue-num 0
```

The router can operate in two different ways. In Drop mode, all packets coming to the router is dropped from iptables, and after the packets have gone through the filter chain, a new packet is created and sent from the router. In Accept mode, the packets are kept in the iptables buffer, and are then either dropped by a filter in the chain, or accepted when the packet reaches the end of the filter chain.

### Drop mode
Drop mode is needed if the content of a packet can change through the filter chain. This means if the router implemets NAT functionallity, it must run in Drop mode. If NAT functionallity is used, incoming packets will be destined for the local host on which the router is running. This means that both the INPUT and the FORWARD iptables chains should be sent to NFQUEUE:
```
iptables -A INPUT -j NFQUEUE --queue-num 0
iptables -A FORWARD -j NFQUEUE --queue-num 0
```
In this case, shaped traffic cannot be destined for the host on which the router is running. This is because packets being shaped are dropped from iptables, and recreated in userspace and then sent to the OUTPUT chain. If the packet is destined for the host, it will then arrive back in the INPUT chain and run endless through the shaper. The host machine will, however, be able to send/receive packets which is not shaped by filtering this traffic around the NFQUEUE in iptables:
```
iptables -A INPUT ! -i eth0 -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -t raw -A OUTPUT ! -s 10.0.2.2/24 -j NOTRACK
```
If no NAT functionalities are used, only traffic from the FORWARD chain needs to be sent to NFQUEUE, making unshaped communication with the local host possible without such iptables filters.

### Accept mode
Accept mode enables the router to shape packets for the same host on which it is running. However, this mode does not support altering packet content (and by extention no NAT support). Since this will keep packets in the iptables buffer, and then accept or drop based on the filter chain, this can be added to any iptables chain:
```
iptables -A INPUT -j NFQUEUE --queue-num 0
iptables -A FORWARD -j NFQUEUE --queue-num 0
iptables -A OUTPUT -j NFQUEUE --queue-num 0
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
The main function creates an instance of the Router class, configures it. If the `ipv6` option is set, the main function calls the `execute6()`function, otherwise `execute()` is called (The main function could be extended to run both in seperate threads for dualstack). The Router class then starts receiving packets from the nf queue. When a packet is received, it is dropped from the nf queue if running in Drop mode, otherwise, the packet is send through a series of filters which are classes extending the Filter class. Each filter can alter the packet (only in drop mode), and choose wether or not to send it to the next filter in the series. If all filters choose to forward the packet, it ends in the Output filter which will send the packet to the network through the iptables OUTPUT chain. If a filter does not forward a packet to the next filter, it must call the `setVerdict(false)` function of the packet to drop it. Currently, 7 filters exists not counting the Output filter:

  * The Nat filter implements port restricted nat. It also supports port forwarding.
  * The Loss filter randomly drops packets based on a given probability.
  * The StaticDelay filter delays all packets with a static delay.
  * The DynamicDelay filter delays all packets according to a specified pattern.
  * The DynamicLoss filter drops packets according to a specified pattern.
  * The Burst filter buffers all incoming packets for a given amount of time, and then forwards all simultaniously
  * The TokenBucketFilter is used to impose bandwidth limitations

Running ./router -h yields:
```
Allowed options:
  -h [ --help ]             Print usage message
  --ext-if arg              The external interface of the router node, used to 
                            destinquish ingoing from outgoing traffic.
  --nat-ext-ip arg          The ip of the external interface of the router 
                            (Required with Nat filter)
  --nat-int-ip arg          The ip of the internal interface of the router 
                            (Required with Nat filter)
  --nat-type arg            The type of NAT to be applied, valid types: portr, 
                            addrr, fullcone, symnat
  --nat-timeout arg         The timeout of NAT connections in seconds, 10min 
                            default
  -d [ --delay ] arg        The network delay in ms to be imposed
  -l [ --loss ] arg         The packet loss probability
  --tbf-in-rate arg         The rate limit in kbit pr. sec. for the token 
                            bucket filter
  --tbf-in-max-tokens arg   The maximum amount of tokens for the token bucket 
                            filter
  --tbf-in-max-packets arg  The packet queue length of the token bucket filter
  --tbf-in-red-start arg    The packet queue length at which random early drop 
                            kick in
  --tbf-out-rate arg        The rate limit in kbit pr. sec. for the token 
                            bucket filter
  --tbf-out-max-tokens arg  The maximum amount of tokens for the token bucket 
                            filter
  --tbf-out-max-packets arg The packet queue length of the token bucket filter
  --tbf-out-red-start arg   The packet queue length at which random early drop 
                            kick in
  --dyn-delays arg          Delay values for the dynamic delay filter
  --dyn-delay-res arg       The time resolution of the delay values for delay 
                            filter
  --dyn-losses arg          Loss values for the dynamic loss filter
  --dyn-loss-times arg      time values for the dynamic loss filter
  --burst-dur arg           The time in ms when packets are collected for 
                            bursts
  --burst-sleep arg         The time in ms between bursts
  -e [ --nat-ext-port ] arg External port numbers for port forwarding
  -i [ --nat-int-port ] arg Internal port numbers for port forwarding, if 
                            defined must have same length as nat-ext-port, if 
                            not defined nat-ext-port will be reused for 
                            internal ports
  --filters arg             Ordered list of filters to use, valid filters are: 
                            StaticDelay, Loss, Nat, Burst, TokenBucketFilter, 
                            DynamicDelay, DynamicLoss
  --ipv6                    use IPv6 instead of IPv4
  --accept-mode             run the router in accept mode instead of drop mode
```
The filters argument determines which filters to use. If this option is used, only the filters listed will be added to the filter chain, and will be added in the order they are listed. If this option is not used, all filters are added in the order: Loss, Nat, Burst, Delay, TokenBucketFilter, DynamicDelay, DynamicLoss. Options for a filter which is not added is ignored silently.

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

### DynamicDelay
The DynamicDelay filter is similar to the StaticDelay filter, except imposed delays are not constant. The DynamicDelay filter is configured using the two options `dyn-delays` and `dyn-delay-res`. `dyn-delay-res` determines the time resolution used to determine delay fix points. `dyn-delays` determines the delay value at each fix point.
If `dyn-delays` is set to `"20 40 30"` with a `dyn-delay-res` set to 2, the delay will be 20ms when the router starts, 2 seconds later it will be 40ms, at 4 seconds it will be 30ms, and at 6 seconds it resets to 20ms.
between fix points, the delay is extrapolated liniarly with a 1 second resolution.
Meaning at 1 seconds, the delay is 30ms, at 3 seconds it is 35ms, and at 5 seconds it is 25ms.

### DynamicLoss
The DynamicLoss filter implements packet losses which change according to a configured pattern. This filter is configured with the options `dyn-losses` and `dyn-loss-times`. Both options are given as arrays which must have the same length. `--dyn-losses 0 0.1 0.5 --dyn-loss-times 2 1 3` means the filter will impose 0% packet loss for 2 seconds, then a 10% packet loss for 1 second, then 50% packet loss for 3 senconds, and then repeat the pattern.

## Testing in Docker
The router is developed for testing in a docker container. The test folder on the repository contains an example of using the router in a docker-compose setup. In the test folder are several subfolders: 
* `compose`
* `ping_node`
* `router_node`
* `stun_test`
* `ipv6Compose`
The `router_node` folder contains the Dockerfile building a docker container for the router software which is connected to the Internet, and to an internal network. The script `router.sh` is executed in the container when the docker-compose starts it. This script starts tcpdump to log all network packets, it then sets up the needed iptables rules, and finally starts the router.
The `ping_node` folder contains the Dockerfile building a docker container for a node on the internal network behind the router. In this node, the ping.sh script can be executed which again logs traffic using tcpdump. It then pings `google.com` to show delay and loss are working properly, but also that ICMP and UDP works. It will then download the `index.html` file at `www.google.com` to show TCP works.
The compose folder contains the docker-compose.yml file which configures the two nodes as well as the needed networks. The `tester.sh` script can be used to run the example with the command ```./tester.sh test```. Each step performed by this command can also be done individually. ```./tester.sh create``` will copy the compiled router software from the build folder created earlier, both Docker containers are then build, and finally they are started. ```./tester.sh run_test``` will execute the ping.sh script in the ping_node running the test. ```./tester.sh clean_up``` will save logs from the containers, stop the containers and remove them. 

Port forwarding is not tested with the test performed by this example as this would require a third container. To test it manually, the example makes a nat rule which opens port 5201. Running ```./tester.sh create``` starts the containers, then running ```docker-compose exec pinger bash``` and then run ```iperf3 -s``` on the ping_node. Then from another terminal window, run ```iperf3 -c X.X.X.X``` where X.X.X.X is the external IP address of the router node.

The `stun_test` folder contains a docker-compose setup for running stun tests to determine if the NAT filter works properly.

The `ipv6Compose` folder contains a docker-compose setup for IPv6 testing. In this setup, the `router_node` runs NAT64 to enable the test to run on PC's without IPv6 Internet access. The traffic shaping router is then run on the `ping_node` shaping all traffic on the INPUT and OUTPUT chains in accept-mode.

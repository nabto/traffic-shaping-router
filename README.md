# traffic shaping router

The Traffic shaping router is a linux router designed to emulate different network behaviour for the purpose of testing software. The router requires that traffic in the iptables INPUT and FORWARD chains is sent to the NFQUEUE number 0, it will then send packets on the OUTPUT chain, which must have connection tracking turned of for traffic coming from the internal network:

```
iptables -A FORWARD -j NFQUEUE --queue-num 0
iptables -A INPUT -j NFQUEUE --queue-num 0
iptables -t raw -A OUTPUT ! -s 10.0.2.2/24 -j NOTRACK
```

When the Router receives a packet from nf queue, it is saved in memory, and dropped from nf queue. The packet is then sent through a series of filters. A filter can alter or drop a packet. If a packet is not dropped, the filter forwards the packet to the next filter in the series, ending in the Output filter which will send the packet through the iptables OUTPUT chain.  Currently, 3 filters exists not counting the Output filter:

  * The Loss filter randomly drops packets based on a given probability.
  * The Nat filter implements port restricted nat. It also supports port forwarding.
  * The StaticDelay filter delays all packets with a static delay.

## Building the Router
Clone the git repository to your machine:
```
git clone https://github.com/nabto/traffic-shaping-router.git
```
Go to the repository, and create a build directory:
```
cd traffic-shaping-router
mkdir build
cd build
```
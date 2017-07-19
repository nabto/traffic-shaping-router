## stun test setup


```
Docker0 <-> portrnat router(internet) <-> stunserver
                                          client router <-> client
```

the stunserver will have two ip addresses such that a full stun test can be made.

The internet network is 10.42.0.0/24
The client network is 10.42.1.0/24

The stun server has two ip addresses 10.42.0.103 and 10.42.0.104

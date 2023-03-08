# 20220520 CPP Long Distance Rabbit publishing

Tests (and times) ability to use the C++ lib `amqpcpp` to publish data to a far-away RabbitMQ broker

## Objective

Show how good the async/event-based approach that this library uses.  As a prerequisite you should set up a "far away" rabbit server e.g one that has a high ping time from wherever you're running the test.  Note that this is not a formal benchmark or "heavy duty" metrics app.  It is a sort of one-off batch of CPP code just to experiment with a concept.  The performance characteristics of this informal test were good enough to convince me to adopt AMQP-CPP in real world projects, where one of the factors was "how good will it work with non-local brokers?"  

## Building

Need to have libamqpcpp installed on the machine.  At Least On My Machine (TM) this worked:

`g++ -o test.out main.cpp -std=c++11 -lamqpcpp -lpthread -ldl -lev -lssl`

## Running
You will need to know the connection string of your AMQP instance.  It should start with `amqp://` or `amqps://`.

```
# Single param mode - just specify the AMQP address.
./test.out amqp://127.0.0.1:3301

## Optional mode - specify address and desired message count (default = 1000)
./test.out amqp://127.0.0.1:3301 1001
```

## Explaining the experiment
The experiment involved connecting a deliberately far-away RabbitMQ instance.  By "far away" I mean in another continent to try to create a high ping time between the test PC and the RabbitMQ broker.  This allows us to test a high round-trip-time (RTT) situation and see for ourselves how the magic of non-blocking / async can help mitigate the high RTTs.

Here is some sample output.

```
# Let's see our ping to the broker.
$ ping X.X.X.X
PING X.X.X.X (X.X.X.X) 56(84) bytes of data.
64 bytes from X.X.X.X: icmp_seq=1 ttl=254 time=181 ms
64 bytes from X.X.X.X: icmp_seq=2 ttl=254 time=177 ms
64 bytes from X.X.X.X: icmp_seq=3 ttl=254 time=175 ms
64 bytes from X.X.X.X: icmp_seq=4 ttl=254 time=176 ms
64 bytes from X.X.X.X: icmp_seq=5 ttl=254 time=185 ms
^C
--- X.X.X.X ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 4063ms
rtt min/avg/max/mdev = 175.455/178.984/185.178/3.657 ms

# Now let's run the app
$ ./test.out amqp://devuser:devpass@X.X.X.X:30123 10
Will connect to amqp://devuser:devpass@X.X.X.X:30123 and publish 10 messages 
connected
ready
declared queue amq.gen-rXLHVJJmx1E_R1Ui5fzmDw
Timer fired
Sent publish signal for message 0
Sent publish signal for message 1
Sent publish signal for message 2
Sent publish signal for message 3
Sent publish signal for message 4
Sent publish signal for message 5
Sent publish signal for message 6
Sent publish signal for message 7
Sent publish signal for message 8
Sent publish signal for message 9
Publish operation accepted by lib for 10 in 2 ms
Got ack on 0
Got ack on 1
Got ack on 2
Got ack on 3
Got ack on 4
Got ack on 5
Got ack on 6
Got ack on 7
Got ack on 8
Got ack on 9
Got ACK on all publishes in 177 ms
The end to end runtime of this app took: 2994 ms
```

In the simple run above the following things happened:
1. A ping test to the broker IP shows an average RTT of 178 milliseconds.  We know our broker definitely is not close. 
2. We run the app.  To keep output low for the sake of this example we did only 10 messages (that's the second param on the CLI.)
3. The app connects to the broker, declares a queue, and starts a timer.
4. Ten messages are published in a loop.  We can see that doing the `self->reliable->publish` command only takes 2ms so "practically nothing" (in repeat runs sometimes it is so low that it registers as '0ms').  
5. Callback code tells us when we get an ack for our messages.  In this case we got it after 177ms, which makes sense, because that's pretty close to our RTT time seen on ping.  
6. Due to the way that the looped publishes were batched by the underlying lib and the status of the channel is then polled using libev all the acks come in within that time period.  It is not as if this is a blocking function call where we call, wait and block, block while we receive the result, etc.  

## Credits
This started using the demo shell from the [AMQP-CPP libev.cpp example](https://github.com/CopernicaMarketingSoftware/AMQP-CPP/blob/master/examples/libev.cpp) from the CopernicaMarketingSoftware AMQP-CPP lib.

I modified this to added on the basic publish operations, the timer, etc that were needed for the purposes of this experiment.

The generous contributors at Copernica who made the AMQP-CPP lib licensed it under Apache 2.0 so this repo uses the same license.

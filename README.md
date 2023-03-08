# 20220520 CPP Long Distance Rabbit publishing

Tests (and times) ability to use the C++ lib `amqpcpp` to publish data to a far-away RabbitMQ broker

## Objective

Show how good the async/event-based approach that this library uses.  As a prerequisite you should set up a "far away" rabbit server e.g one that has a high ping time from wherever you're running the test.  


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
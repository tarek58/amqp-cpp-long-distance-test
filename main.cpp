/**
 *  LibEV.cpp
 * 
 *  Test program to check AMQP functionality based on LibEV
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2015 - 2018 Copernica BV
 */

/**
 *  Dependencies
 */
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/reliable.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#include <chrono>

int MESSAGE_COUNT;

/**
 *  Custom handler
 */
class MyHandler : public AMQP::LibEvHandler
{
private:
    /**
     *  Method that is called when a connection error occurs
     *  @param  connection
     *  @param  message
     */
    virtual void onError(AMQP::TcpConnection *connection, const char *message) override
    {
        std::cout << "error: " << message << std::endl;
    }

    /**
     *  Method that is called when the TCP connection ends up in a connected state
     *  @param  connection  The TCP connection
     */
    virtual void onConnected(AMQP::TcpConnection *connection) override 
    {
        std::cout << "connected" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection ends up in a ready
     *  @param  connection  The TCP connection
     */
    virtual void onReady(AMQP::TcpConnection *connection) override 
    {
        std::cout << "ready" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection is closed
     *  @param  connection  The TCP connection
     */
    virtual void onClosed(AMQP::TcpConnection *connection) override 
    {
        std::cout << "closed" << std::endl;
    }

    /**
     *  Method that is called when the TCP connection is detached
     *  @param  connection  The TCP connection
     */
    virtual void onDetached(AMQP::TcpConnection *connection) override 
    {
        std::cout << "detached" << std::endl;
    }
    
    
public:
    /**
     *  Constructor
     *  @param  ev_loop
     */
    MyHandler(struct ev_loop *loop) : AMQP::LibEvHandler(loop) {}

    /**
     *  Destructor
     */
    virtual ~MyHandler() = default;
};

/**
 *  Class that runs a timer
 */
class MyTimer
{
private:
    /**
     *  The actual watcher structure
     *  @var struct ev_io
     */
    struct ev_timer _timer;

    /**
     *  Pointer towards the AMQP channel
     *  @var AMQP::TcpChannel
     */
    AMQP::TcpChannel *_channel;

    /**
     *  Name of the queue
     *  @var std::string
     */
    std::string _queue;

    AMQP::Reliable<>* reliable;


    /**
     *  Callback method that is called by libev when the timer expires
     *  @param  loop        The loop in which the event was triggered
     *  @param  timer       Internal timer object
     *  @param  revents     The events that triggered this call
     */
    static void callback(struct ev_loop *loop, struct ev_timer *timer, int revents)
    {
        std::cout << "Timer fired" << std::endl;
        // retrieve the this pointer
        MyTimer *self = static_cast<MyTimer*>(timer->data);


        // publish a message
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < MESSAGE_COUNT; ++i) {
            // self->_channel->publish
            self->reliable->publish("loadtest", self->_queue, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"  + std::to_string(i))
                .onAck([i, MESSAGE_COUNT, start_time, loop]() {
                    std::cout << "Got ack on " << i << std::endl;

                    if (i == MESSAGE_COUNT - 1) {
                        auto e_time =  std::chrono::high_resolution_clock::now();
                        std::cout << "Got ACK on all publishes in "
                            << std::chrono::duration_cast<std::chrono::milliseconds>(e_time - start_time).count()
                            << " ms"
                            << std::endl;       

                        ev_break(loop, EVBREAK_ONE);
                    }
                })
                .onLost([i]() {
                    std::cout << "got LOST on " << i << std::endl;
                })
                .onError([i](const char *message) {
                    std::cout << "Got ERROR on " << i << " : message = " << message << std::endl;
                });
            std::cout << "Sent publish signal for message " << i << std::endl;
        }

        auto end_time =  std::chrono::high_resolution_clock::now();
        std::cout << "presumably sent publish signal for " << MESSAGE_COUNT << " in " 
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() 
            << " ms" << std::endl;
    }

public:
    /**
     *  Constructor
     *  @param  loop
     *  @param  channel
     *  @param  queue
     */
    MyTimer(struct ev_loop *loop, AMQP::TcpChannel *channel, std::string queue) : 
        _channel(channel), _queue(std::move(queue))
    {
        this->reliable = new AMQP::Reliable<>(*channel);

        // initialize the libev structure
        //ev_timer_init(&_timer, callback, 0.005, 1.005);
        ev_timer_init(&_timer, callback, 1.5, 0.);

        // this object is the data
        _timer.data = this;

        // and start it
        ev_timer_start(loop, &_timer);
    }
    
    /**
     *  Destructor
     */
    virtual ~MyTimer()
    {
        // @todo to be implemented
    }
};


/**
 *  Main program
 *  @return int
 */
int main(int argc, char *argv[])
{
    MESSAGE_COUNT = 1000;
    char* connString = argv[0];
    std::vector<std::string> argList(argv + 1, argv + argc);

    if (argList.size() < 1)
    {
        std::cerr << "Usage: ./test.out [rabbitMQConnectionString]" << std::endl;
        std::cerr << "Usage 2: ./test.out [rabbitMQConnectionString] [iterations]" << std::endl;
        exit(1);
    }

    std::string connectionString = argList[0];
    if (argList.size() > 1)
        MESSAGE_COUNT = stoi(argList[1]);

    std::cout << "Will connect to " << connectionString 
        << " and publish " << MESSAGE_COUNT << " messages " << std::endl;

    auto master_start_time = std::chrono::high_resolution_clock::now();

    // access to the event loop
    auto *loop = EV_DEFAULT;
    
    // handler for libev
    MyHandler handler(loop);

    // init the SSL library
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif

    // make a connection
    AMQP::Address address(connectionString);
//    AMQP::Address address("amqps://guest:guest@localhost/");
    AMQP::TcpConnection connection(&handler, address);
    
    // we need a channel too
    AMQP::TcpChannel channel(&connection);

    channel.declareExchange("loadtest", AMQP::ExchangeType::topic, AMQP::durable);

    // create a temporary queue
    channel.declareQueue(AMQP::exclusive).onSuccess([&connection, &channel, loop](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
        
        // report the name of the temporary queue
        std::cout << "declared queue " << name << std::endl;
        
        // close the channel
        //channel.close().onSuccess([&connection, &channel]() {
        //    
        //    // report that channel was closed
        //    std::cout << "channel closed" << std::endl;
        //    
        //    // close the connection
        //    connection.close();
        //});
        
        // construct a timer that is going to publish stuff
        auto *timer = new MyTimer(loop, &channel, name);
        
        //connection.close();
    });
    
    // run the loop
    ev_run(loop, 0);

    auto master_end_time = std::chrono::high_resolution_clock::now();
    std::cout << "The end to end runtime of this app took: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(master_end_time - master_start_time).count() 
        << " ms" << std::endl;

    // done
    return 0;
}
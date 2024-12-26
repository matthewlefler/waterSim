/*
    name: sockets.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for using ip sockets

    packet -> id for 2 byte | size of size for 1 bytes | size of ??? bytes | msg with a number of bytes equal to size
        
        the id will increment over time meaning generarlly higher ids will be newer
            first byte is for error codes / status codes
            second byte increases per iteration

        size of size is the size of the part holding the size of the rest of the message
            allows for highly varing sizes going from 0 to 2^255 bytes
        
        size of the msg from 1 to 2^255 bytes long
            can hold any number of things
            ie: (float4) = 4 * 8 bytes or 32 bytes per float 4 
        
*/
#include <string>

#define ASIO_STANDALONE 

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace messenger;

class messenger
{
    private:
        string ip;
        string port;

        asio::io_context context;
        asio::ip::udp::endpoint endpoint;
        asio::ip::udp::socket socket;

        int* data;
        int data_len;

        char iteration = 'a'; // use a char because it has a size of 1 byte and accepts ++ anyway, also the character is meaningless 

    public:

    /*
     * the constructor of the messenger class
     * sets up the context, endpoint, and server on the given ip and port;
     * 
    */
    messenger(string ip, int port, asio::io_context io_context)
    {
        // create a context, something to automagically handle different machines/platorm's interfaces

        this->context = io_context;

        // get the address of somewhere to connect to (127.0.0.1 is the local address)
        this->endpoint = asio::ip::udp::endpoint(asio::ip::make_address(ip), port);

        // create socket with this platform's context
        this->server = asio::ip::udp::socket(io_context, this->endpoint);

        this->start_receive();

        this->socket.connect(endpoint);
    }       

    void update_data(float* data, int len)
    {
        this->data = data;
        this->data_len = len;
    }

    void start_receive()
    {
        this->server.async_receive_from
        (
            asio::buffer(recv_buffer_), this->endpoint,
            std::bind(&udp_server::handle_receive, this,
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred)
        );
    }

    void handle_receive(const std::error_code& error,
                        std::size_t /*bytes_transferred*/)
    {

        // The error parameter contains the result of the asynchronous operation. Since we only provide the 1-byte recv_buffer_ to contain the client's request, 
        // the io_context object would return an error if the client sent anything larger. We can ignore such an error if it comes up.
        if (!error)
        {

            // Determine what we are going to send.

            std::shared_ptr<std::string> message(new std::string(make_daytime_string()));

            // We now call ip::udp::socket::async_send_to() to serve the data to the client.

            socket_.async_send_to(asio::buffer(*message), remote_endpoint_,
                                  std::bind(&this->handle_send, this, message,
                                  asio::placeholders::error,
                                  asio::placeholders::bytes_transferred));

            // When initiating the asynchronous operation, and if using std::bind, you must specify only the arguments that match the handler's parameter list. In this program, both of the argument placeholders (asio::placeholders::error and asio::placeholders::bytes_transferred) could potentially have been removed.

            // Start listening for the next client request.

            this->start_receive();

            // Any further actions for this client request are now the responsibility of handle_send().

        }
    }

    void handle_send(std::shared_ptr<std::string> /*message*/,
                     const std::error_code& /*error*/,
                     std::size_t /*bytes_transferred*/)
    {

    }
}

messenger* start_messenger(string ip, int port)
{
    try
    {
        // Create a messenger object to accept incoming client requests, and run the io_context object.

        asio::io_context io_context;
        messenger server(ip, port, io_context);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return &server;
}


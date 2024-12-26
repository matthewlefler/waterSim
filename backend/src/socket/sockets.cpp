/*
    name: sockets.cpp
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for using unix sockets
*/ 
#include <string>
#include <iostream>

#define ASIO_STANDALONE 

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include "sockets.h"

int main()
{
    asio::error_code ec;

    // create a context, something to automagically handle different machines/platorm's interfaces
    asio::io_context context;

    // get the address of somewhere to connect to (127.0.0.1 is the local address)
    asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 80);

    // create socket with this platform's context
    asio::ip::udp::socket socket(context);

    socket.connect(endpoint, ec);

    if(!ec)
    {
        std::cout << "Connected at " << endpoint.address() << std::endl;
    }
    else
    {
        std::cout << "Failed to connect at " << endpoint.address() << " because, " << ec.message() << std::endl;
    }

    return 0;
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




/*
 * the constructor of the messenger class
 * sets up the context, endpoint, and server on the given ip and port;
 * 
*/
messenger::messenger(string ip, int port, asio::io_context)
{
    // create a context, something to automagically handle different machines/platorm's interfaces

    this->context = io_context;

    // get the address of somewhere to connect to (127.0.0.1 is the local address)
    this->endpoint = asio::ip::udp::endpoint(asio::ip::make_address(ip), port);

    // create socket with this platform's context
    this->server = asio::ip::udp::socket(io_context, this->endpoint);

    this->server.async_receive_from(
        asio::buffer(recv_buffer_), this->endpoint,
        std::bind(&udp_server::handle_receive, this,
          asio::placeholders::error,
          asio::placeholders::bytes_transferred));

    this->socket.connect(endpoint);
}

// The function handle_receive() will service the client request.
messenger::handle_receive(const std::error_code& error,
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
                              std::bind(&udp_server::handle_send, this, message,
                              asio::placeholders::error,
                              asio::placeholders::bytes_transferred));

        // When initiating the asynchronous operation, and if using std::bind, you must specify only the arguments that match the handler's parameter list. In this program, both of the argument placeholders (asio::placeholders::error and asio::placeholders::bytes_transferred) could potentially have been removed.

        // Start listening for the next client request.

        start_receive();

        // Any further actions for this client request are now the responsibility of handle_send().

    }
}

void handle_send(std::shared_ptr<std::string> /*message*/,
      const std::error_code& /*error*/,
      std::size_t /*bytes_transferred*/)
{

}



void update_data(float* data, int len)
{
    this->data = data;
    this->data_len = len;
}
/*
    name: sockets.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for using ip sockets

    packet 1 -> id for 2 byte | size of size for 1 bytes | size of msg for 1021 bytes || totals 1024 for the header message 
    packet 2 -> msg with a number of bytes equal to size
        
        the id will increment over time meaning generarlly higher ids will be newer
            first byte is for error codes / status codes
            second byte increases per iteration

        size of size is the size of the part holding the size of the rest of the message
            allows for highly varing sizes going from 0 to 2^255 bytes
        
        size of the msg from 1 to 2^255 bytes long
            can hold any number of things
            ie: (float4) = 4 * 8 bytes or 32 bytes per float 4 

       -1 is some type of error
       

        0 is base
        1 is get sim data -> aka width depth height the stats (standerdize this in the "future" lol)
        ...
        255 is ??
        
*/
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"

#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/IPAddress.h"

#include "sycl/sycl.hpp"

const int send_buffer_length = 1024*10;

class EchoConnection: public Poco::Net::TCPServerConnection 
{
    private:
        sycl::host_accessor<float, 1, sycl::access_mode::read> accessor;
        sycl::range<3> dims;

        int array_length;
        int size_of_array_length;
        static constexpr int size_of_data_type = sizeof(float);

        char iter = 'a';

    public:
        EchoConnection(const Poco::Net::StreamSocket& s, sycl::host_accessor<float, 1, sycl::access_mode::read> accessor, sycl::range<3> dims): TCPServerConnection(s) 
        {
            this->accessor = accessor;
            this->dims = dims;

            this->array_length = dims.get(0) * dims.get(1) * dims.get(2) * 3;
            this->size_of_array_length = sizeof(array_length);
         }

    void run() 
    {
        Poco::Net::StreamSocket& ss = socket();

        // used to exit on an abnormal shutdown of the endpoint/error condition of the socket
        bool exit = false;
        while(!exit)
        {
            try {
                char buffer[3];
                int n = ss.receiveBytes(buffer, sizeof(buffer));

                float send_buffer[send_buffer_length]; // size is arbitrary rn as idk what the max is and what it should be 

                int number_of_bytes_to_send = size_of_data_type * array_length; // amount of bytes required to model the data in the arr

                int bytes_sent = 0;


                switch (int(buffer[0]))
                {
                case 0:
                    send_buffer[0] = 0;
                    send_buffer[1] = iter++;

                    send_buffer[2] = size_of_array_length;

                    std::memcpy(&send_buffer[3], &array_length, size_of_array_length);

                    ss.sendBytes(&send_buffer, 1024); // has to match the receiveing buffer for the header msg in the receiver 
                                                      // as it is possible that if different the socket will read into the data and "delete" some data and offset the rest of the data

                    if(send_buffer_length > number_of_bytes_to_send) // if the data an be sent in one send call, do so
                    {
                        for (int i = 0; i < array_length; i++)
                        {                           
                            send_buffer[i] = accessor[i];
                        }

                        bytes_sent += ss.sendBytes(&send_buffer, size_of_data_type * array_length);
                    }
                    else //split the data up into different send calls 
                    {
                        int j = 0;

                        while(bytes_sent < number_of_bytes_to_send)
                        {
                            for(int i = 0; i < send_buffer_length / size_of_data_type; i++)
                            {
                                send_buffer[i] = accessor[j];

                                j++;
                            }

                            bytes_sent += ss.sendBytes(&send_buffer, send_buffer_length);
                        }
                    }

                    break;
                
                case 1:
                    // send data relating to the structure of the simulation
                    // aka: the width height depth, tall cells, combined cells, etc.
                    send_buffer[0] = 1;
                    send_buffer[1] = iter++;

                    send_buffer[2] = 3;

                    send_buffer[3] = this->dims.get(0); // width
                    send_buffer[4] = this->dims.get(1); // height
                    send_buffer[5] = this->dims.get(2); // depth 

                    ss.sendBytes(&send_buffer, 6);

                    break;

                case 255:
                    exit = true;
                    break;

                case -1: // unsure what or how the first byte becomes -1 if the frontend is terminated/closed, but handle it anyway, probs due to a unexpected shutdown of the endpoint  ¯\_(ツ)_/¯
                    exit = true;
                    break;

                default:
                    std::cerr << "in sockets.h, EchoConnection: unhandled first received byte: " << buffer[0] << "\n";
                    break;
                }            
            }
            catch(Poco::IOException& exc)
            {
                std::cerr << "in sockets.h, EchoConnection: " << exc.displayText() << std::endl; 
                exit = true;
            }
            catch (Poco::Exception& exc)
            { 
                std::cerr << "in sockets.h, EchoConnection: " << exc.displayText() << std::endl; 
                exit = true;
            }
        }
    }
};

class TCPServerConnectionFactoryTheSecond: public Poco::Net::TCPServerConnectionFactory
{
    private:
        sycl::host_accessor<float, 1, sycl::access_mode::read> accessor;
        sycl::range<3> dims;

    public:
        TCPServerConnectionFactoryTheSecond(sycl::host_accessor<float, 1, sycl::access_mode::read> accessor, sycl::range<3> dims)
        {
            this->accessor = accessor;
            this->dims = dims;
        }

        Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket)
        {
            return new EchoConnection(socket, accessor, dims);
        }
}; 

class Messenger
{
    private:
        Poco::Net::TCPServer* server;

    public:

    Messenger(Poco::UInt16 port, sycl::host_accessor<float, 1, sycl::access_mode::read> accessor, sycl::range<3> dims)
    {
        server = new Poco::Net::TCPServer(new TCPServerConnectionFactoryTheSecond(accessor, dims), port);
        server->start(); // start the async server object

        std::cout << "starting server at address: " << server->socket().address().toString() << " | with a send_buffer size of: " << send_buffer_length << " bytes " << "\n";
    }

    ~Messenger()
    {
        std::cout << "stopping server at address: " << this->server->socket().address().toString() << "\n";

        this->server->stop(); // stop the server 
        free(server); // and free the memory
    }

    void UpdateData()
    {
        //TODO: update the data somehow
    }
};

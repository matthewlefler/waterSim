/**
    name: sockets.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for using ip sockets

    packet 1 -> id for 2 byte | size of size for 1 bytes | size of msg for ?? bytes | totals 1024 bytes
    packet 2 to n -> msg with a number of bytes equal to size
        
        the id will increment over time meaning generarlly higher ids will be newer
            first byte is for error codes / status codes
            second byte increases per iteration

        size of size is the size of the part holding the size of the rest of the message
            allows for highly varing sizes going from 0 to 2^255 bytes
        
        size of the msg from 0 to 2^255 bytes long
            can hold any number of things
            ie: (float4) = 4 * 8 bytes or 32 bytes per float 4 

        
        0 is base
        1 is get sim data -> aka width depth height the stats (standerdize this in the "future" lol)
        ...
        255 is ??

    the "server" spawns connections that handle each connection in an async context
        
*/

#include <string>
#include <iostream>
#include <algorithm>
#include <atomic>

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/SocketAddressImpl.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/IPAddress.h"

const int send_buffer_length = 1024*10;

template<typename T>
class EchoConnection: public Poco::Net::TCPServerConnection 
{
    private:
        std::atomic<T *> * arr; // a pointer to the array; 
        int number_of_bytes_to_send; // the length of the array in bytes

        int size_of_data_type = sizeof(T);
        int size_of_number_of_bytes_to_send; // amount of bytes required to model the data in the arr 

        int width;
        int height;
        int depth;

        char iter = 'a';

    public:
        EchoConnection(const Poco::Net::StreamSocket& s, std::atomic<T *> * pointer_to_arr, int width, int height, int depth): TCPServerConnection(s) 
        {
            this->arr = pointer_to_arr;

            this->width = width;
            this->height = height;
            this->depth = depth;

            this->number_of_bytes_to_send = width * height * depth * size_of_data_type;
            this->size_of_number_of_bytes_to_send = sizeof(this->number_of_bytes_to_send);
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

                char send_buffer[send_buffer_length]; // size is arbitrary rn as idk what the max is and what it should be 

                int bytes_sent = 0;

                switch (int(buffer[0]))
                {
                case 0:
                    send_buffer[0] = 0;
                    send_buffer[1] = ++iter;

                    send_buffer[2] = size_of_number_of_bytes_to_send;

                    std::memcpy(&send_buffer[3], &number_of_bytes_to_send, size_of_number_of_bytes_to_send);

                    ss.sendBytes(&send_buffer, 1024); // has to match the receiveing buffer for the header msg in the receiver 
                                                      // as it is possible that if different the socket will read into the data
                                                      // and "delete" some data and offset the rest of the data

                    if(send_buffer_length > number_of_bytes_to_send) // if the data an be sent in one send call, do so
                    {    
                        std::memcpy(send_buffer, arr->load(), number_of_bytes_to_send);

                        bytes_sent += ss.sendBytes(&send_buffer, number_of_bytes_to_send);
                    }
                    else //split the data up into different send calls 
                    {
                        int j = 0;

                        while(bytes_sent < number_of_bytes_to_send)
                        {
                            for(int i = 0; i < send_buffer_length / size_of_data_type; ++i)
                            {
                                std::memcpy(&send_buffer[(i * size_of_data_type)], &(arr->load()[j]), size_of_data_type);
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
                    send_buffer[1] = ++iter;

                    send_buffer[2] = 3;

                    send_buffer[3] = this->width; // width
                    send_buffer[4] = this->height; // height
                    send_buffer[5] = this->depth; // depth 

                    ss.sendBytes(&send_buffer, 6);

                    break;

                case 255: // standard shutdown of the client
                    exit = true;
                    break;

                case -1: // unsure what or how the first byte becomes -1 if the frontend is terminated/closed, but handle it anyway
                         // abnormal shutdown of the client
                    exit = true;
                    break;

                default:
                    std::cerr << "in sockets.h, EchoConnection: unhandled first received byte" << buffer[0] << "\n";
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

template<typename T>
class TCPServerConnectionFactoryTheSecond: public Poco::Net::TCPServerConnectionFactory
{
    private:
        std::atomic<T *> * arr;
        int width;
        int height;
        int depth;

    public:
        TCPServerConnectionFactoryTheSecond(std::atomic<T *> & arr, int width, int height, int depth)
        {
            this->arr = &arr;
            this->width = width;
            this->height = height;
            this->depth = depth;
        }

        Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket)
        {
            std::cout << "\nnew connection from: " << socket.address().toString() << "\n";
            return new EchoConnection<T>(socket, arr, width, height, depth);
        }
}; 

template<typename T>
class Messenger
{
    private:
        Poco::Net::TCPServer* server;

    public:

    Messenger(Poco::UInt16 port, std::atomic<T*> & arr, int width, int height, int depth)
    {
        server = new Poco::Net::TCPServer(new TCPServerConnectionFactoryTheSecond<T>(arr, width, height, depth), port);
        server->start();

        std::cout << "starting server at address: " << server->socket().address().toString() << " | with a send_buffer size of: " << send_buffer_length << " bytes " << "\n";
    }

    ~Messenger()
    {
        std::cout << "stopping server at address: " << this->server->socket().address().toString() << "\n";

        this->server->stop(); // stop the server 
        free(server); // and free the memory
    }
};


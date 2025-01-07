/*
    name: sockets.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for using ip sockets

    packet -> id for 2 byte | size of size for 1 bytes | size of msg for ?? bytes | msg with a number of bytes equal to size
        
        the id will increment over time meaning generarlly higher ids will be newer
            first byte is for error codes / status codes
            second byte increases per iteration

        size of size is the size of the part holding the size of the rest of the message
            allows for highly varing sizes going from 0 to 2^255 bytes
        
        size of the msg from 1 to 2^255 bytes long
            can hold any number of things
            ie: (float4) = 4 * 8 bytes or 32 bytes per float 4 

        
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

#define DATATYPE float4 // size of 16 bytes (4 floats x,y,z,w consisting of 4 bytes each in a row)

class EchoConnection: public Poco::Net::TCPServerConnection 
{
    private:
        DATATYPE* arr;
        int arr_len;

        int size_of_arr_len;
        int size_of_data_type;

        char iter = 'a';

    public:
        EchoConnection(const Poco::Net::StreamSocket& s, DATATYPE* arr, int arr_len): TCPServerConnection(s) 
        {
            this->arr = arr;
            this->arr_len = arr_len;
            this->size_of_arr_len = sizeof(arr_len);

            this->size_of_data_type = sizeof(DATATYPE);
         }

    void run() 
    {
        Poco::Net::StreamSocket& ss = socket();

        bool exit = false;
        while(!exit)
        {
            try {
                char buffer[3];
                int n = ss.receiveBytes(buffer, sizeof(buffer));

                char send_buffer[1024]; // size is arbitrary rn as idk what the max is and what it should be 

                switch (int(buffer[0]))
                {
                case 0:
                    send_buffer[0] = 0;
                    send_buffer[1] = iter++;

                    send_buffer[2] = size_of_arr_len;

                    std::memcpy(&send_buffer[3], &arr_len, size_of_arr_len);

                    for (int i = 0; i < arr_len; i++)
                    {
                        std::memcpy(&send_buffer[2 + size_of_arr_len + (i * size_of_data_type)], &arr[i], size_of_data_type);

                    }
                    
                    ss.sendBytes(&send_buffer, 1024);
                    
                    break;
                
                case 1:
                    send_buffer[0] = 1;
                    send_buffer[1] = iter++;

                    send_buffer[2] = 3;

                    send_buffer[3] = 5; //width
                    send_buffer[4] = 5; //height
                    send_buffer[5] = 5; //depth (unused for now)

                    ss.sendBytes(&send_buffer, 6);

                    break;

                case 255:
                    exit = true;
                    break;

                case -1: //unsure what or how the first byte becomes -1 if the frontend is terminated/closed, but handle it anyway
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
            }
        }
    }
};

class TCPServerConnectionFactoryTheSecond: public Poco::Net::TCPServerConnectionFactory
{
    private:
        DATATYPE* arr;
        int arr_len;

    public:
        TCPServerConnectionFactoryTheSecond(DATATYPE* arr, int arr_len)
        {
            this->arr = arr;
            this->arr_len = arr_len;
        }


        Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket)
        {
            return new EchoConnection(socket, arr, arr_len);
        }
}; 

class Messenger
{
    private:
        Poco::Net::TCPServer* server;

    public:

    Messenger(Poco::UInt16 port, DATATYPE* arr, int arr_len)
    {
        server = new Poco::Net::TCPServer(new TCPServerConnectionFactoryTheSecond(arr, arr_len), port);
        server->start();

        std::cout << "starting server at address: " << server->socket().address().toString() << "\n";
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



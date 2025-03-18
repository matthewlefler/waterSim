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

    first byte id number and meaning 
    0 -> a test of connection aka is the other side on and working? similar to a ping, should receive: "hi, I am: {name of computer}"
         only includes the first two bytes b/c the rest is unnessesary, (may help find bugs later too) 
    1 -> normal data of the (currently water) simulation
    255 -> this messenger is no longer on and should be removed from lists including it. 
*/

// A C# program for Client sockets
using System;
using System.Net;
using System.Net.Sockets;

namespace Messengers;

public class Messenger<T>
{
    readonly byte[] basicMessage = 
    { 
        0 
    };

    readonly byte[] closingMessage = 
    { 
        255 
    };

    readonly byte[] getData = 
    { 
        1
    };

    private IPAddress ipAddr;
    private IPEndPoint endPoint;

    private Socket socket;

    private int simWidth = 0;
    private int simHeight = 0;
    private int simDepth = 0;

    public bool connected = false;

    private Func<byte[], T[]> converter_func;
    private Action<T[], int, int, int> set_action;

    public Messenger(int port, Func<byte[], T[]> converter_func, Action<T[], int, int, int> set_action) 
    {
        this.converter_func = converter_func;
        this.set_action = set_action;

        // Establish the remote endpoint 
        // for the socket. This example 
        // uses port {port} on the local 
        // computer.
        Console.WriteLine("Will connect to: " + IPAddress.Loopback.ToString() + " and port: " + port.ToString());
        this.ipAddr = IPAddress.Loopback;
        this.endPoint = new IPEndPoint(ipAddr, port);

        // Creation UDP/IP Socket using 
        // Socket Class Constructor
        this.socket = new Socket(this.ipAddr.AddressFamily,
                   SocketType.Stream, ProtocolType.Tcp);   //<--- tcp and type
    }

    /// <summary>
    /// try to connect to the port and address set when initializing this messenger
    /// </summary>
    public void connect()
    {    
        try {

            // Connect Socket to the remote 
            // endpoint using method Connect()
            this.socket.Connect(this.endPoint);

            int byteSent = this.socket.Send(getData);

            // Data buffer
            byte[] messageReceived = new byte[1024];

            // We receive the message using 
            // the method Receive(). 
            int byteRecv = this.socket.Receive(messageReceived);

            this.simWidth = messageReceived[3];
            this.simHeight = messageReceived[4];
            this.simDepth = messageReceived[5];

            connected = true;
            Console.WriteLine($"Messenger connected at {this.socket.RemoteEndPoint.ToString()}");
        }

        // Manage the Socket's Exceptions
        catch
        {
            // continue 
        }
    }

    /// <summary>
    /// close the socket 
    /// should be done whenever the application is closing or the socket is no longer needed
    /// </summary>
    public void close()
    {
        if(connected)
        {
            this.socket.Send(closingMessage);
        }
        
        // Close Socket using 
        // the method Close()
        this.socket.Shutdown(SocketShutdown.Both);
        this.socket.Close();
    }

    /// <summary>
    /// pokes the server for data and then sets the sim's data to the decoded received data
    /// if the transfer of data failes or is corrupted no data is changed and the function returns immediatly
    /// </summary>
    public void read() 
    {
        if(!connected) { return; }

        this.socket.Send(basicMessage);

        // Data buffer
        byte[] buffer = new byte[1024];

        // We receive the header message using 
        // the method Receive(). This method 
        // returns number of bytes received
        int byteRecv;
        
        while(this.socket.Available == 0)
        {
            //wait untill data is avalible
        }

        try
        {
            byteRecv = this.socket.Receive(buffer);
        }
        catch
        {
            // Console.WriteLine(e.ToString());
            return;
        }

        int sizeOfSize = buffer[2];

        byte[] sizeArr = { 0, 0, 0, 0, 0, 0, 0, 0 }; 

        for (int i = 0; i < sizeOfSize; i++)
        {
            sizeArr[i] = buffer[i + 3];
        }

        int data_byte_size = BitConverter.ToInt32(sizeArr);
        
        //receive the data of the msg
        // counts the number of times data is recieved
        byte[] data = new byte[data_byte_size];
        int current_index = 0;
        
        int timeoutCount = 0;
        int totalByteRecived = 0;
        while(totalByteRecived < data_byte_size)
        {
            int avalible_bytes = this.socket.Available;

            if(avalible_bytes > 0)
            {
                buffer = new byte[this.socket.Available];
                try
                {
                    byteRecv = this.socket.Receive(buffer, buffer.Length, SocketFlags.None);
                    totalByteRecived += byteRecv;
                    
                    for(int i = 0; i < byteRecv; i++)
                    {
                        if(current_index >= data_byte_size) { break; }

                        data[current_index] = buffer[i];
                        current_index++;
                    }

                    timeoutCount = 0;
                }
                catch
                {
                    // Console.WriteLine(e.ToString());
                    return;
                }
            }

            timeoutCount++;

            if(timeoutCount > 1000)
            {
                return;
            }
        }


        // init an appropiately sized 1 dimenional array to hold the decrypted data
        T[] sim_data = new T[this.simWidth * this.simHeight * this.simDepth];

        // convert the byte array to the type array
        sim_data = converter_func(data);

        // call the action to do something with the decrypted data
        set_action(sim_data, simWidth, simHeight, simDepth);
    }

#nullable disable
}


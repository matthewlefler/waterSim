/*   
packet -> id for 2 byte | size of size for 1 bytes | size of ??? bytes | msg with a number of bytes equal to size
    
    the id will increment over time meaning generarlly higher ids will be newer
        first byte is for error codes / status codes
        second byte for id number (for debugging, count time since last, etc.)

    size of size is the size of the part holding the size of the rest of the message
        allows for highly varing sizes going from 0 to 2^255 bytes
    
    size of the msg from 1 to 2^255 bytes long
        can hold any number of things
        ie: (float4) = 4 * 8 bytes or 32 bytes per float 4 

    first byte id
    0 -> a test of connection aka is the other side on and working? similar to a ping, should receive: "hi, I am: {name of computer}"
         only includes the first two bytes b/c the rest is unnessesary, (may help find bugs later too) 
    1 -> normal data of the (currently water) simulation
    255 -> this messenger is no longer on and should be removed from lists including it. 
*/

// A C# program for Client sockets
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Security.Claims;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Xna.Framework;
using Objects;

namespace Messengers;

public class Messenger
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

    public Messenger(int port) 
    {
        // Establish the remote endpoint 
            // for the socket. This example 
            // uses port {port} on the local 
            // computer.
            Console.WriteLine("Connecting at: " + IPAddress.Loopback.ToString() + " and port: " + port.ToString());
            this.ipAddr = IPAddress.Loopback;
            this.endPoint = new IPEndPoint(ipAddr, port);
    }

    /// <summary>
    /// try to connect to the port and address set when initializing this messenger
    /// </summary>
    public void connect()
    {
        try {
            // Creation UDP/IP Socket using 
            // Socket Class Constructor
            this.socket = new Socket(this.ipAddr.AddressFamily,
                       SocketType.Stream, ProtocolType.Tcp);                       //<--- tcp and type
    
            try {

                // Connect Socket to the remote 
                // endpoint using method Connect()
                this.socket.Connect(this.endPoint);
    
                // We print EndPoint information 
                // that we are connected
                Console.WriteLine("Socket connected to -> {0} ",
                              this.socket.RemoteEndPoint.ToString());

                int byteSent = this.socket.Send(getData);
    
                // Data buffer
                byte[] messageReceived = new byte[1024];
    
                // We receive the message using 
                // the method Receive(). This 
                // method returns number of bytes
                // received, that we'll use to 
                // convert them to string
                int byteRecv = this.socket.Receive(messageReceived);

                this.simWidth = messageReceived[3];
                this.simHeight = messageReceived[4];
                this.simDepth = messageReceived[5];

                connected = true;
            }

            // Manage of Socket's Exceptions
            catch (ArgumentNullException ane) 
            {
                // Console.WriteLine("ArgumentNullException : {0}", ane.ToString());
            }

            catch (SocketException se) 
            {
                // Console.WriteLine("SocketException : {0}", se.ToString());
            }

            catch (Exception e) 
            {
                // Console.WriteLine("Unexpected exception : {0}", e.ToString());
            }

        }

        catch (Exception e) 
        {
            // Console.WriteLine(e.ToString());
        }
    }

    /// <summary>
    /// close the socket 
    /// should be done whenever the application is closing or the socket is no longer needed
    /// </summary>
    public void close()
    {

        this.socket.Send(closingMessage);
        
        // Close Socket using 
        // the method Close()
        this.socket.Shutdown(SocketShutdown.Both);
        this.socket.Close();
    }

    /// <summary>
    /// pokes the server for data and then sets the sim's data to the decoded received data
    /// if the transfer of data failes or is corrupted no data is changed and the function returns immediatly
    /// </summary>
    /// <param name="sim"> the simulation which holds the data to change </param>
    public void read(Simulation sim) // tuple of the sim and a bool of if the read worked
    {
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
        catch (Exception e) 
        {
            // Console.WriteLine(e.ToString());
            return;
        }

        int sizeOfSize = buffer[2];

        byte[] sizeArr = new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 }; 

        for (int i = 0; i < sizeOfSize; i++)
        {
            sizeArr[i] = buffer[i + 3];
        }

        int data_size = BitConverter.ToInt32(sizeArr);
        int data_byte_size = data_size * 16;

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
                catch (Exception e) 
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

        // init an appropiately sized 2 dimenional array to hold the decrypted data
        Vector4[] sim_data = new Vector4[this.simWidth * this.simHeight * this.simDepth];

        Vector4 temp;
        for (int i = 0, j = 0; j < data_size; i+=16, j++)
        {
            // float = 4 bytes
            temp.X = BitConverter.ToSingle(data, i); // i to i + 3 
            temp.Y = BitConverter.ToSingle(data, i + 4); // i + 4 to i + 7
            temp.Z = BitConverter.ToSingle(data, i + 8); // i + 8 to i + 11
            temp.W = BitConverter.ToSingle(data, i + 12); // i + 12 to i + 15

            sim_data[j] = temp;
        }

        sim.SetData(sim_data, this.simWidth, this.simHeight, this.simDepth);
    }

#nullable disable
}


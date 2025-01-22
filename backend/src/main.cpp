/*
    name: main.cpp
    author: matt l
        slack: @skye 

    usecase:
        the main file that runs the simulation and handels posting and recieving from any unix sockets
*/ 
#include "simulation_class.h"
#include "main.h"
#include "socket/sockets.h"

#include <string>
#include <iostream>

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>
using namespace sycl;

int main()
{
    // set up memory
    // initilize the simulation
    Simulation sim(10, 10, 10);

    //Messenger messenger = Messenger(4331, &sim.vectors, *sim.pWidth, *sim.pHeight, *sim.pDepth);

    std::cout << *sim.pWidth << " " << *sim.pHeight << " " << *sim.pDepth << " " << *sim.pAmtOfCells << "\n";

    // print the beginning state of the vectors (for testing purposes)
    // also assign them a value of their position followed by rand % 2
    for (int i = 0; i < *sim.pAmtOfCells; ++i)
    {
        int x = sim.getXPos(i);
        int y = sim.getYPos(i);
        int z = sim.getZPos(i);

        sim.vectors[i] = float4(x, y, z, rand() % 2);
    }
    std::cout << "set vectors \n";

    sim.send();
    std::cout << "sent vectors to device \n";

    int count = 0;
    bool exit = false;
    while(true)
    {
        std::cout << "here0?\n"; 

        sim.next_frame(1.0f);
        std::cout << "frame: " << count << "\n";
        //sim.print();

        if(count > 200) { exit = true; }

        if(exit)
        {
            // quit the fun loop
            break;
        }

        sleep(1);
        ++count;
    }

    std::cout << "\n";

    return 0;
}



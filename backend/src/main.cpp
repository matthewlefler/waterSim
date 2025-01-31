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
    
    Messenger messenger = Messenger(4331, sim.vectors, *sim.pWidth, *sim.pHeight, *sim.pDepth);

    std::cout << "width: " << *sim.pWidth << " height: " << *sim.pHeight << " depth: " << *sim.pDepth << " total number of cells: " << *sim.pAmtOfCells << "\n";

    // assign the stating value of the simulation
    for (int i = 0; i < *sim.pAmtOfCells; ++i)
    {
        int x = sim.getXPos(i);
        int y = sim.getYPos(i);
        int z = sim.getZPos(i);

        sim.vectors[i] = float4(0.0f, 2.5f, 1.0f, 1.0f);

        if(y == 0)
        {
            sim.vectors[i].w() = 0.0f;
        }
    }

    sim.send();

    int count = 0;
    bool exit = false;
    while(true)
    {
        std::cout << "frame: " << count << "\n";
        sim.next_frame(1.0f);
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



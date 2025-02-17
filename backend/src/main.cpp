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
#include <atomic>

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>

int main()
{
    // set up memory
    // initilize the simulation
    Simulation sim(10, 10, 10);
    
    Messenger messenger = Messenger(4331, sim.vectors, *sim.pWidth, *sim.pHeight, *sim.pDepth);

    std::cout << "width: " << *sim.pWidth << " height: " << *sim.pHeight << " depth: " << *sim.pDepth << " total number of cells: " << *sim.pAmtOfCells << "\n";

    float r = 2;
    // assign the stating value of the simulation
    for (int i = 0; i < *sim.pAmtOfCells; ++i)
    {
        int x = sim.getXPos(i);
        int y = sim.getYPos(i);
        int z = sim.getZPos(i);

        sim.vectors[i] = sycl::_V1::float4(0.0f, 0.0f, 0.0f, 1.0f);
        sim.changeable[i] = true;

        if(y == 0 || y == *sim.pWidth - 1 || z == 0 || z == *sim.pDepth - 1)
        {
            sim.vectors[i].x() = 0.0f;
            sim.vectors[i].y() = 0.0f;
            sim.vectors[i].z() = 0.0f;

            sim.changeable[i] = false;
        }

        if(x*x + y*y < r*r)
        {
            sim.vectors[i].x() = 0.0f;
            sim.vectors[i].y() = 0.0f;
            sim.vectors[i].z() = 0.0f;
            
            sim.changeable[i] = false;
        }
    }

    sim.send();

    int count = 0;
    std::atomic<bool> exit = std::atomic<bool>();
    exit = false;

    while(true)
    {
        // TODO: add proper timing and fps counter for stats
        std::cout << "frame: " << count << "\n";
        sim.next_frame(1.0f);
        //sim.print();

        if(count > 200) { exit = true; }

        if(exit.load())
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



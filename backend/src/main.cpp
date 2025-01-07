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

float round(float value, int places)
{
    float mult = sycl::_V1::pow(10.0f, (float)places);
    float newValue = (float) sycl::_V1::floor(value * mult) / mult;
    return newValue;
}

int main()
{
    // set up memory
    // initilize the simulation
    Simulation sim(5, 5);

    Messenger messenger = Messenger(4331, sim.vectors, *sim.pTotalLength);

    std::cout << *sim.pWidth << " " << *sim.pHeight << " " << *sim.pTotalLength << "\n";

    // print the beginning state of the vectors (for testing purposes)
    // also assign them a value of their index followed by three zeros
    for (int i = 0; i < *sim.pTotalLength; ++i)
    {
        int x = i % *sim.pWidth;
        int y = i / *sim.pHeight;

        sim.vectors[i] = float4(x, y, i, 0);
        std::cout << " | " << round(sim.vectors[i].x(), 2) << " " << round(sim.vectors[i].y(), 2) << " " << round(sim.vectors[i].z(), 2) << " " << round(sim.vectors[i].w(), 2) << " ";

        if(i % *sim.pWidth + 1 >= *sim.pWidth)
        {
            std::cout << "\n";
        }
    }
    std::cout << "\n";

    sim.send();


    int count  = 0;
    bool exit = false;
    while(true)
    {
        sim.next_frame(1.0f);
        sim.print();

        if(count > 50) { exit = true; }

        if(exit)
        {
            // quit the fun loop
            break;
        }

        sleep(1);
        ++count;
    }
    
    // print the final state of the vectors (for testing purposes)
    sim.print();

    std::cout << "\n";

    return 0;
}



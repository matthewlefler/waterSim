/*
    name: main.cpp
    author: matt l
        slack: @skye 

    usecase:
        the main file that runs the simulation and handels posting and recieving from any unix sockets
*/ 
#include "simulation_class.h"
#include "main.h"
#include "sockets.h"

#include <string>
#include <iostream>

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>
using namespace sycl;

int main()
{
    //set up memory
    simulation sim = simulation(5, 5);


    std::cout << *sim.pWidth << " " << *sim.pHeight << " " << *sim.pTotalLength << "\n";

    // print the beginning state of the vectors (for testing purposes)
    // also assign them a value of their index followed by three zeros
    for (int i = 0; i < *sim.pTotalLength; ++i)
    {
        sim.vectors[i] = float4(0,0,0,i);
        std::cout << " | " << sim.vectors[i].x() << " " << sim.vectors[i].y() << " " << sim.vectors[i].z() << " " << sim.vectors[i].w() << " ";

        if(i % *sim.pWidth + 1 >= *sim.pWidth)
        {
            std::cout << "\n";
        }
    }
    std::cout << "\n";

    sim.send();



    bool exit = false;
    while(true)
    {
        sim.next_frame(1.0f);

        exit = true;

        if(exit)
        {
            // quit the fun loop
            break;
        }
    }
    
    // print the final state of the vectors (for testing purposes)
    for (int i = 0; i < *sim.pTotalLength; ++i)
    {
        std::cout << " | " << sim.vectors[i].x() << " " << sim.vectors[i].y() << " " << sim.vectors[i].z() << " " << sim.vectors[i].w() << " ";

        if(i % *sim.pWidth + 1 >= *sim.pWidth)
        {
            std::cout << "\n";
        }
    }
    std::cout << "\n";

    return 0;
}
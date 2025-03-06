/*
    name: main.cpp
    author: matt l
        slack: @skye 

    usecase:
        the main file that runs the simulation and handels posting and recieving from any unix sockets
*/ 
#include "simulation/simulation_class.hpp"
#include "socket/sockets.hpp"

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
    
    // Messenger messenger = Messenger(4331, sim.get_host_access(), sim.get_dimensions());

    sycl::range<3> tempDims = sim.get_dimensions();
    std::cout << "width: " << tempDims.get(0) << " height: " << tempDims.get(1) << " depth: " << tempDims.get(2) << "\n";

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



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
    //                                   unused   unused    unused          unused     
    //             width, height, depth, density, visocity, speed_of_sound, node_size, cyc_radius, tau
    Simulation sim(10, 10, 10, 0.1f, 0.1f, 0.1f, 1.0f, 2.0f, 0.8f);

    sycl::range<3> tempDims = sim.get_dimensions();
    
    //                                                     port #, data pointer,   width,           height,          depth
    Messenger velocity_messenger = Messenger<sycl::float4>(4000, sim.vector_array, tempDims.get(0), tempDims.get(1), tempDims.get(2));
    Messenger density_messenger = Messenger<float>(4001, sim.density_array, tempDims.get(0), tempDims.get(1), tempDims.get(2));
    // for sending and recevieing data about the simulation conditions, and for receiveing commands from the frontend
    // Messenger communication_messenger = Messenger<int>(4002, sim.density_array, tempDims.get(0), tempDims.get(1), tempDims.get(2)); // does not work

    std::cout << "simulation: width is " << tempDims.get(0) << ", height is " << tempDims.get(1) << ", depth is " << tempDims.get(2) << "\n";

    int count = 0;
    std::atomic<bool> exit = std::atomic<bool>();
    exit.store(false);

    while(true)
    {
        // TODO: add proper timing and fps counter for stats
        std::cout << "\n/////////////\n";
        std::cout << "// frame " << count << " //\n";
        std::cout << "/////////////\n" << std::endl;

        sim.next_frame();

        if(count > 1000) { exit.store(true); }

        if(exit.load())
        {
            // quit the fun loop
            break;
        }

        sleep(1); // !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!! REMOVE IF NOT DEBUGGING !!!
        ++count;
    }

    std::cout << "\n";

    return 0;
}
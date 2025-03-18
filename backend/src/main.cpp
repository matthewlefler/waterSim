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

    sycl::range<3> tempDims = sim.get_dimensions();
    
    //                                                     port #, data pointer,   width,           height,          depth
    Messenger velocity_messenger = Messenger<sycl::float4>(4000, sim.vector_array, tempDims.get(0), tempDims.get(1), tempDims.get(2));
    Messenger density_messenger = Messenger<float>(4001, sim.density_array, tempDims.get(0), tempDims.get(1), tempDims.get(2));

    // messenger test 
    // sycl::float4 * tempTestData = new sycl::float4[2];
    // tempTestData[0] = sycl::float4(1.0f, 1.0f, 1.0f, 1.0f);
    // tempTestData[1] = sycl::float4(0.0f, 0.0f, 0.0f, 0.0f);
    // Messenger messenger = Messenger(4331, tempTestData, 2, 1, 1);

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

        sim.next_frame(1.0f);

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

    // free(tempTestData); // free messenger test temp data
    return 0;
}
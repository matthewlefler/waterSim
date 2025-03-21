/*
    name: main.cpp
    author: matt l
        slack: @skye 

    usecase:
        a secondary main file that saves data to a file to be read in later
*/ 
#include "simulation/simulation_class.hpp"
#include "socket/sockets.hpp"

#include <string>
#include <iostream>
#include <atomic>

#include <fstream>

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>


void write_to_file(std::ofstream & file, Simulation & sim) 
{
    auto density_accessor = sim.get_accessor_for_discrete_density_buffer_1();
    auto changeable_accessor = sim.get_accessor_for_changeable_buffer();
    for(int i = 0; i < sim.get_node_count(); ++i) 
    {
        file << (int) changeable_accessor[i] << " ";

        float density = sim.density_array.load()[i];
        file << int(density * 100.0f) / 100.0f << " ";

        for(uint8_t j = 0; j < 27; ++j)
        {
            float val = density_accessor[i * 27 + j];
            val = int(val * 1000.0f) / 1000.0f;
            file << val << " ";
        }
    }
    file << "\n";
}

std::string filename = "test.txt";
int main()
{
    std::cout << "writing to file: " << filename << std::endl;

    std::ofstream file;
    file.open(filename, std::ofstream::out | std::ofstream::trunc);

    if(!file.is_open())
    {
        std::cerr << "file: " << filename << "could not be opened" << std::endl;
        return 1;
    }

    // set up memory
    // initilize the simulation
    Simulation sim(130, 1, 150, 1.225f, 0.00001f, 343, 0.02f, 55.0f);

    sycl::range<3> temp_dims = sim.get_dimensions();

    std::cout << "simulation: width is " << temp_dims.get(0) << ", height is " << temp_dims.get(1) << ", depth is " << temp_dims.get(2) << "\n";

    // write the dimentions to the top line in the file
    file << temp_dims.get(0) << " " << temp_dims.get(1) << " " << temp_dims.get(2) << "\n"; 

    int count = 0;
    std::atomic<bool> exit = std::atomic<bool>();
    exit.store(false);

    while(true)
    {
        // TODO: add proper timing and fps counter for stats
        std::cout << "// frame " << count << " //\n";

        sim.next_frame(1.0f);

        write_to_file(file, sim);

        if(count > 500) { exit.store(true); }

        if(exit.load())
        {
            // quit the fun loop
            break;
        }

        ++count;
    }

    file.close();

    std::cout << "\n---data written successfully---\n\n";

    return 0;
}


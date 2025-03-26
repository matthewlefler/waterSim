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

#include <fstream> // write to files
#include <chrono> // get the time it took to run the simulation

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
int main(int argc, char *argv[])
{
    if(argc < 7 || argc > 7)
    {
        std::cout << "usage: " << argv[0] << " number_of_frames_to_compute sim_width sim_height sim_depth tau_value cylinder_radius" << std::endl;
        return 0;
    }

    std::cout << "writing to file: " << filename << std::endl;

    std::ofstream file;
    file.open(filename, std::ofstream::out | std::ofstream::trunc);

    if(!file.is_open())
    {
        std::cerr << "file: " << filename << "could not be opened" << std::endl;
        return 1;
    }

    // get the total number of frames to compute from the command line arguments
    int number_of_frames_to_compute = std::stoi(argv[1]);

    // set up memory
    // initilize the simulation          unused   unused    unused          unused
    //             width, height, depth, density, visocity, speed_of_sound, node_size, cyc_radius, tau
    Simulation sim(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), 1.225f, 0.00001f, 343, 0.02f, std::stof(argv[6]), std::stof(argv[5]));

    sycl::range<3> temp_dims = sim.get_dimensions();
    
    std::cout << "simulation: width is " << temp_dims.get(0) << ", height is " << temp_dims.get(1) << ", depth is " << temp_dims.get(2) << "\n";

    // write the dimentions to the top line in the file
    file << temp_dims.get(0) << " " << temp_dims.get(1) << " " << temp_dims.get(2) << "\n"; 

    int current_frame_number = 0;
    std::atomic<bool> exit = std::atomic<bool>();
    exit.store(false);

    long sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    while(true)
    {
        write_to_file(file, sim);
        
        sim.next_frame();


        if(current_frame_number > number_of_frames_to_compute) { exit.store(true); }

        if(exit.load())
        {
            // quit the fun loop
            break;
        }

        ++current_frame_number;
    }

    file.close();

    sec = std::chrono::duration_cast<std::chrono::milliseconds> ( std::chrono::system_clock::now().time_since_epoch() ).count() - sec;

    std::cout << "\ntook " << sec / 1000.0f << " seconds\n";
    std::cout << "\n---data written successfully---\n\n";

    return 0;
}


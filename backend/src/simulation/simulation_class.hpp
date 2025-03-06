/*
    name: simulation_class.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for a custom simulation class to model incompressible fluids and gas
*/ 
#include <iostream> // used for debugging via std out
#include <atomic> // will use for having two copies of the velocity buffers and having an atomic bool to switch between them
#include <stdint.h> // used for the better defined types such as int8_t and int32_t

#include "float4_helper_functions.hpp" // a few helper functions such as dot product, magnitude etc. for the xyz componenets of a float4
#include "sim_setup_helper_functions.hpp" // a few helper functional objects to setup the sim in various configurations

#include <sycl/sycl.hpp> // the main library used for parrellelism 

//<-- TEMPERARY DEBUGGING FUNCTION !-->
// Our simple asynchronous handler function
auto handle_async_error = [](sycl::exception_list elist) {
    for (auto &e : elist) {
        try{ std::rethrow_exception(e); }
        catch ( sycl::exception& e ) {
            // Print information about the asynchronous exception
        }
    }
    // Terminate abnormally to make clear to user
    // that something unhandled happened
    std::terminate();
};

const float c = 1.0f;
inline float f_eq(float weight, float density, float velocity_i_x, float velocity_i_y, float velocity_i_z, float macro_velocity_x, float macro_velocity_y, float macro_velocity_z) 
{
    return weight * density * ( 1
    + ((3 * (velocity_i_x * macro_velocity_x + velocity_i_y * macro_velocity_y + velocity_i_z * macro_velocity_z)) / c*c)
    + ((9 * (velocity_i_x * macro_velocity_x + velocity_i_y * macro_velocity_y + velocity_i_z * macro_velocity_z) * (velocity_i_x * macro_velocity_x + velocity_i_y * macro_velocity_y + velocity_i_z * macro_velocity_z)) / (2 * c*c*c*c))
    - ((3 * (macro_velocity_x * macro_velocity_x + macro_velocity_y * macro_velocity_y + macro_velocity_z * macro_velocity_z)) / (2 * c*c))
    );
}

/**
 * this simulation uses the lattice boltzmann method (LBM) of computational fluid dynamics, currently a d3q27 (3 dimensional, 27 discrete velocities)
 * 
 * the simulation is made up of 
 *      a. nodes (points) in 3s space (which are enclosed in a rectangular prism)
 * 
 *      b. and of a number of velocities, often denoted as i in papers on this topic for some specific velocity of the total number, 
 *         these are the speeds that a particle at a particluar node can have 
 * 
 * the simulation works in two steps that are done for each call of the next_frame function 
 * 
 * 1. streaming (aka advection)
 * 2. collision
 * 
 * advection is the process of density, aka the particles, moving around from cell to cell based on the predifined possible velocities that they can take
 * 
 * collision is the process of the previously advected particles colliding at their new positions
 * 
 */
class Simulation
{
    private:
        int width;
        int height;
        int depth;

        sycl::queue q;

        // some senagagins but,
        // these are the possible velocities the particles can take,
        // stored in sequential groups of three (x, y, z), sorted from highest weight to lowest weight
        // 
        //      vector number * 3     = index of x value 
        //      vector number * 3 + 1 = index of y value
        //      vector number * 3 + 2 = index of z value
        // 
        // each componenet has a min value of -128, max value of 127
        const int8_t possible_velocities[3 * 27] = {
// vec #     value         dir      name 
/* 0 */      0,  0,  0, //          origin

/* 1 */      1,  0,  0, // x+       unit vectors
/* 2 */      0,  1,  0, //    y+
/* 3 */      0,  0,  1, //       z+
/* 4 */     -1,  0,  0, // x-       inverse unit vectors
/* 5 */      0, -1,  0, //    y- 
/* 6 */      0,  0, -1, //       z-

/* 7  */     1,  1,  0, // x+ y+    xy plane corners
/* 8  */    -1,  1,  0, // x- y+ 
/* 9  */     1, -1,  0, // x+ y- 
/* 10 */    -1, -1,  0, // x- y- 
/* 11 */     0,  1,  1, //    y+ z+ yz plane corners
/* 12 */     0, -1,  1, //    y- z+
/* 13 */     0,  1, -1, //    y+ z-
/* 14 */     0, -1, -1, //    y- z-
/* 15 */     1,  0,  1, // x+    z+ xz plane corners
/* 16 */    -1,  0,  1, // x-    z+
/* 17 */     1,  0, -1, // x+    z-
/* 18 */    -1,  0, -1, // x-    z-

/* 19 */     1,  1,  1, // x+ y+ z+ 3d corners
/* 20 */     1,  1, -1, // x+ y+ z-
/* 21 */     1, -1,  1, // x+ y- z+
/* 22 */     1, -1, -1, // x+ y- z-
/* 23 */    -1,  1,  1, // x- y+ z+
/* 24 */    -1,  1, -1, // x- y+ z-
/* 25 */    -1, -1,  1, // x- y- z+
/* 26 */    -1, -1, -1, // x- y- z-
        };

        static const int32_t possible_velocities_number = 27;

        sycl::buffer<int8_t, 1> * possible_velocities_buffer; 


        // these are the weights that each velocity has
        // the weights add up to 1
        const float velocities_weights[27] = {
            8.0f / 27.0f, // for 1

            2.0f / 27.0f, // for 6
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 

            1.0f / 54.0f, // for 12
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 
            1.0f / 54.0f, 

            1.0f / 216.0f, // for 8
            1.0f / 216.0f, 
            1.0f / 216.0f, 
            1.0f / 216.0f, 
            1.0f / 216.0f,
            1.0f / 216.0f,
            1.0f / 216.0f,
            1.0f / 216.0f,
        };

        sycl::buffer<float, 1> * velocities_weights_buffer; 

        /////////////////////////////////////////
        // density information                 //
        // wrapped in buffers (memory objects) //
        /////////////////////////////////////////
        
        sycl::buffer<bool, 3> * changeable_buffer; 

        // densities for each of the (27) velocities per position node. 
        // flattened into a 1d "array" of floats 
        // index = x + y * 3 + z * 9 + node_index
        // node_index = (node.x + node.y * width + node.z * width * height) * 27
        sycl::buffer<float, 1> * velocity_vectors_buffer_1; // the old values for f(x, t, e_i) aka the values to read from    
        sycl::buffer<float, 1> * velocity_vectors_buffer_2; // the new values for f(x, t, e_i) aka the values to write to

        // the three dimensional dimensions of the simulation, 
        // the real size of one dimension length is the length of that dimension times the ref_len
        sycl::range<3> * dims;
        sycl::range<1> * velocity_buffer_length;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // these four reference vars allow for an a-dimentioned Lattice Boltzmann Method (LBM) solver //
        // this also allows for the solver to cover more types of fluids, more easily                 //
        ////////////////////////////////////////////////////////////////////////////////////////////////

        // reference length in meters, 
        // is also the distance between two nodes
        // and the width, height, depth of a node
        float ref_len = 1.0f; 

        // reference length in seconds 
        float ref_time = 1.0f; 

        // reference density in ??
        float ref_density = 1.0f; 

        // reference speed in meters per second (m/s)
        float ref_speed = ref_time / ref_density; 
        
        // this changes the time it takes for the fluid to relax back to the equlibrium state
        // is related semi-directly to the fluid's viscosity 
        float tau = 1.0f;

        ///////////////////////////////////////////////
        // macroscopic variables                     //
        // Used in the collision operator of the LBM //
        ///////////////////////////////////////////////
        
        float density;
        float macro_velocity_x; 
        float macro_velocity_y;
        float macro_velocity_z;

    public:
        // x and y and z velocity vector components + w for density
        float* vectors;

    //constructor
    Simulation(int width, int height, int depth)
    {
        this->q = sycl::queue(sycl::cpu_selector_v);
        std::cout << "running simulation on -> " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        this->dims = new sycl::range<3>(width, height, depth);
        this->velocity_buffer_length = new sycl::range<1>(width * height * depth * possible_velocities_number);

        this->possible_velocities_buffer = new sycl::buffer<int8_t, 1>(this->possible_velocities, possible_velocities_number * 3);
        this->velocities_weights_buffer = new sycl::buffer<float, 1>(this->velocities_weights, possible_velocities_number);

        this->changeable_buffer         = new sycl::buffer<bool,  3>(*this->dims);
        this->velocity_vectors_buffer_1 = new sycl::buffer<float, 1>(*this->velocity_buffer_length);
        this->velocity_vectors_buffer_2 = new sycl::buffer<float, 1>(*this->velocity_buffer_length);

        this->vectors = new float[velocity_buffer_length->get(0)];

        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);

            h.parallel_for(*this->velocity_buffer_length, [=](sycl::id<1> i) 
            {
                device_accessor_velocity_buffer_1[i] = 0.4f;
            });
        }).wait();

        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<bool, 3, sycl::access_mode::write> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                device_accessor_changeable_buffer[i] = true;

                float r = 0.4f;
                float r_square = r * r;

                if(i.get(0) * i.get(0) + i.get(2) * i.get(2) < r_square)
                {
                    device_accessor_changeable_buffer[i] = false;
                }
            }
            );
        }).wait();
    }

    // deconstructor
    ~Simulation()
    {
        // free the allocated memory in the device
        free(this->changeable_buffer, q); 
        free(this->possible_velocities_buffer, q);
        free(this->velocity_vectors_buffer_1, q); 
        free(this->velocity_vectors_buffer_2, q); 

        free(this->dims);
        free(this->velocity_buffer_length);
    }

    void next_frame(float dt)
    {
        std::cout << "starting done" << std::endl;

        sycl::range<3> local_dims = *this->dims;
        sycl::range<1> local_velocity_buffer_length = *this->velocity_buffer_length;

        float local_density = this->density;

        float local_macro_velocity_x = this->macro_velocity_x;
        float local_macro_velocity_y = this->macro_velocity_y;
        float local_macro_velocity_z = this->macro_velocity_z;

        float local_tau = this->tau;

        int local_possible_velocities_number = this->possible_velocities_number;

        // std::cout << "starting streaming..." << std::endl;

        // do the math
        //event streaming = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> node_position) 
            {
                //                 = x + y * width + z * width * height;
                uint64_t node_index = (
                                      node_position.get(0)
                                    + node_position.get(1) * local_dims.get(0) 
                                    + node_position.get(2) * local_dims.get(0) * local_dims.get(1)
                                    ) * 27; // WARNING: supports up to a cube of ~ 880_748 by 880_748 by 880_748 nodes

                // loop over all the vectors and grab the particles that will move to the current node,
                // and assign them to the associated velocity on the current node
                // also skip the velocity with value (0, 0, 0) as it does not change
                for (uint8_t i = 1; i < local_possible_velocities_number; i++)
                {
                    // if the position to get the particles from is out of bounds, skip it (the better way is to write 26 more for loops to remove the if statement (for the 12 edges 8 corners 6 faces + this))
                    if (
                        (node_position.get(0) - device_accessor_possible_velocities[i * 3])     < 0 ||
                        (node_position.get(1) - device_accessor_possible_velocities[i * 3 + 1]) < 0 ||
                        (node_position.get(2) - device_accessor_possible_velocities[i * 3 + 2]) < 0 ||

                        (node_position.get(0) - device_accessor_possible_velocities[i * 3])     > local_dims.get(0) - 1 ||
                        (node_position.get(1) - device_accessor_possible_velocities[i * 3 + 1]) > local_dims.get(1) - 1 ||
                        (node_position.get(2) - device_accessor_possible_velocities[i * 3 + 2]) > local_dims.get(2) - 1   
                    ) { continue; }

                    // get where the particles are coming from
                    uint64_t from_node_index = (
                                               (node_position.get(0) - device_accessor_possible_velocities[i * 3])
                                             + (node_position.get(1) - device_accessor_possible_velocities[i * 3 + 1]) * local_dims.get(0) 
                                             + (node_position.get(2) - device_accessor_possible_velocities[i * 3 + 2]) * local_dims.get(0) * local_dims.get(1)
                                            ) * 27;

                    // move the particles to current node with their velocity 
                    device_accessor_velocity_buffer_2[node_index + i] = device_accessor_velocity_buffer_1[from_node_index + i];
                }
            });
        }).wait();

        // std::cout << "streaming done" << std::endl;
        // std::cout << "starting collision..." << std::endl;

        //event collision = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);

            sycl::accessor<bool, 3, sycl::access_mode::read> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);

            h.parallel_for(*this->velocity_buffer_length, [=](sycl::id<1> i) 
            { 
                int local_velocity_index = i % local_possible_velocities_number;

                float weight = device_accessor_velocities_weights[local_velocity_index];

                float e = f_eq(weight, local_density, 
                               device_accessor_possible_velocities[local_velocity_index * 3],     // velocity (e_i) x val
                               device_accessor_possible_velocities[local_velocity_index * 3 + 1], // velocity (e_i) y val
                               device_accessor_possible_velocities[local_velocity_index * 3 + 2], // velocity (e_i) z val
                               local_macro_velocity_x,
                               local_macro_velocity_y,
                               local_macro_velocity_z);

                device_accessor_velocity_buffer_1[i] = device_accessor_velocity_buffer_2[i] 
                                                     + ((e - device_accessor_velocity_buffer_2[i]) / local_tau);
            });
        }).wait();

        std::cout << "collision done" << std::endl;

        // calculate macroscopic variables 
        auto host_access = velocity_vectors_buffer_1->get_host_access();
        // calc the macroscopic density
        density = 0;
        for(int i = 0; i < this->velocity_buffer_length->get(0); ++i)
        {
            density += host_access[i];
        }
        
        // calc the macroscopic velocity (x, y, z)
        for(int i = 0; i < this->velocity_buffer_length->get(0); ++i)
        {
            macro_velocity_x += host_access[i] * possible_velocities[(i % 27) * 3];
            macro_velocity_y += host_access[i] * possible_velocities[(i % 27) * 3 + 1];
            macro_velocity_z += host_access[i] * possible_velocities[(i % 27) * 3 + 2];
        }

        macro_velocity_x = macro_velocity_x / density;
        macro_velocity_y = macro_velocity_y / density;
        macro_velocity_z = macro_velocity_z / density;

        this->q.wait();
    }

    sycl::host_accessor<float, 1, sycl::access_mode::read> get_host_access() 
    {
        return sycl::host_accessor<float, 1, sycl::access_mode::read>(*this->velocity_vectors_buffer_1);
    }

    sycl::range<3> get_dimensions()
    {
        return sycl::range(*this->dims);
    }
};



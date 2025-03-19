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

// will turn on/off related functions that print out degub information
// such as the read_out_values function
const bool DEBUG = true;

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

/*
return weight * density * ( 1 + first + second - third)
first = 3 * (v*u) * 1/c^2
second = 9 * (v*u)^2 * 1/2c^4
third = 3 * (u*u) * 1/2c^2
*/
#define c 1.0f
inline float f_eq(float weight, float density, float velocity_i_x, float velocity_i_y, float velocity_i_z, float macro_velocity_x, float macro_velocity_y, float macro_velocity_z) 
{
    float vdotu = velocity_i_x * macro_velocity_x + velocity_i_y * macro_velocity_y + velocity_i_z * macro_velocity_z;
    float udotu = macro_velocity_x * macro_velocity_x + macro_velocity_y * macro_velocity_y + macro_velocity_z * macro_velocity_z;
    return weight * density * ( 1 + (( 3 * vdotu ) / (c*c)) + (( 9 * vdotu * vdotu ) / (2 * c*c*c*c)) - (( 3 * udotu ) / (2 * c*c)));
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
        const int8_t possible_velocities[81] = {
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

        // used for collisions where the node is a reflective boundary node,
        // where the changeable_buffer value of the node is equal to 1
        const uint8_t relective_index_table_new[27] = {
// vec #            x   y   z          x   y   z
/* 0 */     0,  //  0,  0,  0 maps to  0,  0,  0

/* 1 */     4,  //  1,  0,  0 maps to -1,  0,  0
/* 2 */     5,  //  0,  1,  0 maps to  0, -1,  0
/* 3 */     6,  //  0,  0,  1 maps to  0,  0, -1
/* 4 */     1,  // -1,  0,  0 maps to  1,  0,  0
/* 5 */     2,  //  0, -1,  0 maps to  0,  1,  0
/* 6 */     3,  //  0,  0, -1 maps to  0,  0,  1

/* 7  */    10, //  1,  1,  0 maps to -1, -1,  0
/* 8  */    9,  // -1,  1,  0 maps to  1, -1,  0
/* 9  */    8,  //  1, -1,  0 maps to -1,  1,  0
/* 10 */    7,  // -1, -1,  0 maps to  1,  1,  0
/* 11 */    14, //  0,  1,  1 maps to  0, -1, -1
/* 12 */    13, //  0, -1,  1 maps to  0,  1, -1
/* 13 */    12, //  0,  1, -1 maps to  0, -1,  1
/* 14 */    11, //  0, -1, -1 maps to  0,  1,  1
/* 15 */    18, //  1,  0,  1 maps to -1,  0, -1
/* 16 */    17, // -1,  0,  1 maps to  1,  0, -1
/* 17 */    16, //  1,  0, -1 maps to -1,  0,  1
/* 18 */    15, // -1,  0, -1 maps to  1,  0,  1

/* 19 */    26, //  1,  1,  1 maps to -1, -1, -1
/* 20 */    25, //  1,  1, -1 maps to -1, -1,  1
/* 21 */    24, //  1, -1,  1 maps to -1,  1, -1
/* 22 */    23, //  1, -1, -1 maps to -1,  1,  1
/* 23 */    22, // -1,  1,  1 maps to  1, -1, -1
/* 24 */    21, // -1,  1, -1 maps to  1, -1,  1
/* 25 */    20, // -1, -1,  1 maps to  1,  1, -1
/* 26 */    19  // -1, -1, -1 maps to  1,  1,  1
        };

        sycl::buffer<uint8_t, 1> * relective_index_table_new_buffer; 


        const float flow_vec_x = 0.0f;
        const float flow_vec_y = 0.0f;
        const float flow_vec_z = 1.0f;

        /////////////////////////////////////////
        // density information                 //
        // wrapped in buffers (memory objects) //
        /////////////////////////////////////////
        
        // if a node is a boundary node,
        // the value corisponds to the type of boundary node it is
        // 
        // 0 = not a boundary node (do normal equlibrium collision)
        // 1 = a reflective boundary (reflect particles)
        // 2 = in/out flow (equal to the variables: flow_vec_x, flow_vec_y, flow_vec_z)
        sycl::buffer<uint8_t, 1> * changeable_buffer; 

        // densities for each of the (27) velocities per position node. 
        // flattened into a 1d "array" of floats 
        // index = x + y * 3 + z * 9 + node_index
        // node_index = (node.x + node.y * width + node.z * width * height) * 27
        sycl::buffer<float, 1> * discrete_density_buffer_1; // the old values for f(x, t, e_i) aka the values to read from    
        sycl::buffer<float, 1> * discrete_density_buffer_2; // the new values for f(x, t, e_i) aka the values to write to

        // the three dimensional dimensions of the simulation, 
        // the real size of one dimension length is the length of that dimension times the ref_len
        sycl::range<3> * dims;
        sycl::range<1> * discrete_density_buffer_length;

        // number of descrete positions in the simulation 
        // equal to the width of the sim * the height * the depth
        sycl::range<1> * node_count;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // these four reference vars allow for an a-dimentioned Lattice Boltzmann Method (LBM) solver //
        // this also allows for the solver to cover more types of fluids, more easily                 //
        ////////////////////////////////////////////////////////////////////////////////////////////////

        // reference length in meters, 
        // is also the distance between two nodes
        float ref_len = 1.0f; 

        // reference length in seconds 
        float ref_time = 1.0f; 

        // reference density in ??
        float ref_density = 1.0f; 

        // reference speed in meters per second (m/s)
        float ref_speed = ref_time / ref_density; 
        
        // this changes the time it takes for the fluid to relax back to the equlibrium state
        // is related semi-directly to the fluid's viscosity 
        // a value above 0 and less than approx 2.5, higher values become unstable
        float tau = 2.1f;

        ///////////////////////////////////////////////
        // macroscopic variables                     //
        // Used in the collision operator of the LBM //
        ///////////////////////////////////////////////
        
        float * density_array_1; 
        float * density_array_2; 

        sycl::buffer<float, 1> * macro_density_buffer; // the macroscopic density (one per node)
        sycl::buffer<float, 1> * macro_velocity_x; // the x component of the macroscopic velocity (one per node)
        sycl::buffer<float, 1> * macro_velocity_y; // the y component of the macroscopic velocity (one per node)
        sycl::buffer<float, 1> * macro_velocity_z; // the z component of the macroscopic velocity (one per node)

        // copy 1 of the vectors data  
        sycl::float4* vectors1;
        // copy 2 of the vectors data  
        sycl::float4* vectors2;

        sycl::buffer<sycl::float4, 1> * vectors;

        // which array is currently pointed to by the vector_array pointer
        // false means vectors1 is pointed to by vector_array
        // true means vectors2 is pointed to by vector_array
        bool which_vectors_array = false;

    public:
        /////////////////////////////////////////////////////////////////////////
        // stable host instances of the macroscopic velocity and density array //
        /////////////////////////////////////////////////////////////////////////

        // a value containing a pointer to the current macroscopic velocity array
        std::atomic<sycl::float4*> vector_array; 
        // a value containing a pointer to the current macroscopic density array
        std::atomic<float*> density_array; 

    //constructor
    Simulation(int width, int height, int depth)
    {
        this->q = sycl::queue(sycl::cpu_selector_v);
        std::cout << "running simulation on -> " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        this->dims = new sycl::range<3>(width, height, depth);
        this->discrete_density_buffer_length = new sycl::range<1>(width * height * depth * possible_velocities_number);
        this->node_count = new sycl::range<1>(width * height * depth);

        // constant values, 
        // a list of the velocities per node
        this->possible_velocities_buffer = new sycl::buffer<int8_t, 1>(this->possible_velocities, possible_velocities_number * 3);
        // the weights asscociated with each velocity
        this->velocities_weights_buffer  = new sycl::buffer<float, 1>(this->velocities_weights, possible_velocities_number);
        // the reflected possible velocity index for a given possible velocity index
        this->relective_index_table_new_buffer = new sycl::buffer<uint8_t, 1>(this->relective_index_table_new, possible_velocities_number);

        // if this node is a boundary node, and which type is it
        this->changeable_buffer         = new sycl::buffer<uint8_t, 1>(*this->node_count);
        // a list of the paricle amounts for each descrete velocity for each node
        this->discrete_density_buffer_1 = new sycl::buffer<float, 1>(*this->discrete_density_buffer_length);
        // the second list of the paricle amounts for each descrete velocity for each nod
        this->discrete_density_buffer_2 = new sycl::buffer<float, 1>(*this->discrete_density_buffer_length);

        // macrosopic varibles, used in the equlibrium density function defined above this class
        this->macro_density_buffer = new sycl::buffer<float, 1>(*this->node_count);

        this->macro_velocity_x = new sycl::buffer<float, 1>(*this->node_count);
        this->macro_velocity_y = new sycl::buffer<float, 1>(*this->node_count);
        this->macro_velocity_z = new sycl::buffer<float, 1>(*this->node_count);


        // host side density arrays
        this->density_array_1 = new float[this->node_count->get(0)];
        this->density_array_2 = new float[this->node_count->get(0)];
        
        // public facing macro density array
        this->density_array.store(density_array_1);
        
        // public facing macro velocity array
        this->vectors = new sycl::buffer<sycl::float4, 1>(*this->node_count);

        // host side velocity arrays
        this->vectors1 = new sycl::float4[this->node_count->get(0)];
        this->vectors2 = new sycl::float4[this->node_count->get(0)];

        this->vector_array = vectors1;

        // initalize the descrete density buffer 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);

            h.parallel_for(*this->discrete_density_buffer_length, [=](sycl::id<1> i) 
            {
                device_accessor_discrete_density_buffer_1[i] = device_accessor_velocities_weights[i % 27];

                if(i / 27 == (width / 2) + (height / 2) * width + (depth / 2) * width * height ) 
                {
                    if(i % 27 == 0) { device_accessor_discrete_density_buffer_1[i] = 100.0f; }
                }
            });
        }).wait();
        
        // set which nodes are boundary nodes
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<uint8_t, 1, sycl::access_mode::write> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            // currently a cylinder aligned along the y axis with radius r
            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                int64_t index = i.get(0) + i.get(1) * width + i.get(2) * width * height;
                device_accessor_changeable_buffer[index] = 0;

                // float r = 3.0f;
                // float r_square = r * r;

                // float x = i.get(0) - (width / 2);
                // float z = i.get(2) - (depth / 2);

                // if( x*x + z*z < r_square )
                // {
                //     device_accessor_changeable_buffer[index] = 1;
                // }
                
                // if(i.get(2) == 0 || i.get(2) == depth - 1)
                // {
                //     device_accessor_changeable_buffer[index] = 2;
                // }
            });
        }).wait();
        
        // set up the vectors buffer with the initial values
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);

            sycl::accessor<sycl::float4, 1, sycl::access_mode::write> device_accessor_vectors(*this->vectors, h);

            h.parallel_for(*this->node_count, [=](sycl::id<1> i) 
            { 
                float vec_x = 0.0f;
                float vec_y = 0.0f;
                float vec_z = 0.0f;
                
                for(uint8_t i; i < possible_velocities_number; ++i)
                {
                    vec_x += device_accessor_velocities_weights[i] * device_accessor_possible_velocities[i * 3];
                    vec_y += device_accessor_velocities_weights[i] * device_accessor_possible_velocities[i * 3 + 1];
                    vec_z += device_accessor_velocities_weights[i] * device_accessor_possible_velocities[i * 3 + 2];
                }
                
                device_accessor_vectors[i] = sycl::float4(vec_x, vec_y, vec_z, 0.0f);
            });
        }).wait();

        // prime the two vectors arrays
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

            h.copy(device_accessor_vectors, vectors1);
        }).wait();

        // prime the second vector array
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

            h.copy(device_accessor_vectors, vectors2);
        }).wait();

        // initalize the macro density buffer 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_density_buffer(*this->macro_density_buffer, h);

            h.parallel_for(*this->node_count, [=](sycl::id<1> i) 
            {
                float density = 0.0f;
                for (uint8_t i = 0; i < possible_velocities_number; i++)
                {
                    density += device_accessor_discrete_density_buffer_1[i * 27 + i];
                }

                device_accessor_macro_density_buffer[i] = density;
            });
        }).wait();

        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

            h.copy(device_accessor_density, density_array_1);
        }).wait();

        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

            h.copy(device_accessor_density, density_array_2);
        }).wait();

        // make sure all jobs on the gpu complete
        q.wait();
    }

    // deconstructor
    ~Simulation()
    {
        q.wait();

        // free the allocated memory on the device / host
        free(this->changeable_buffer, q); 
        free(this->relective_index_table_new_buffer, q); 

        free(this->possible_velocities_buffer, q);
        free(this->discrete_density_buffer_1, q); 
        free(this->discrete_density_buffer_2, q); 

        free(this->dims);
        free(this->discrete_density_buffer_length);

        free(this->macro_density_buffer);
        free(this->macro_velocity_x);
        free(this->macro_velocity_y);
        free(this->macro_velocity_z);

        free(this->vectors);

        free(this->vectors1);
        free(this->vectors2);
    }

    /**
     * prints out an amount of values of the given buffer to the std output
     * uses a host accessor to do so and should only be used for debugging purposes
     */
    template <typename T> 
    void read_out_values(sycl::buffer<T, 1> * buffer, int amount) {
        if(!DEBUG) { return; }

        auto accessor = buffer->get_host_access();
        for (int i = 0; i < amount; i++)
        {
            std::cout << "n" << i << ": " << accessor[i] << ", ";
        }
        // flush the buffer
        std::cout << std::endl;
    }

    /**
     * prints out an amount of values of the given buffer to the std output
     * uses a host accessor to do so and should only be used for debugging purposes
     * 
     * a more specific overload for int8_t (signed char) buffers
     * casting the values to an integer for readablility purposes
     */
    void read_out_values(sycl::buffer<int8_t, 1> * buffer, int amount) {
        if(!DEBUG) { return; }

        auto accessor = buffer->get_host_access();
        for (int i = 0; i < amount; i++)
        {
            std::cout << "n" << i << ": " << (int) accessor[i] << ", ";
        }
        // flush the buffer
        std::cout << std::endl;
    }

    void next_frame(float dt)
    {
        std::cout << "starting done" << std::endl;

        int local_possible_velocities_count = this->possible_velocities_number;

        sycl::range<3> local_dims = *this->dims;
        sycl::range<1> local_velocity_buffer_length = *this->discrete_density_buffer_length;

        float local_tau = this->tau;

        float local_flow_vec_x = this->flow_vec_x;
        float local_flow_vec_y = this->flow_vec_y;
        float local_flow_vec_z = this->flow_vec_z;


        // read out macroscopic variables for debugging purposes
        read_out_values(this->discrete_density_buffer_1, 27);

        std::cout << "starting streaming..." << std::endl;

        //event streaming = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_velocity_buffer_2(*this->discrete_density_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> node_position) 
            {
                //                  = x + y * width + z * width * height;
                uint64_t node_index = (
                                      node_position.get(0)
                                    + node_position.get(1) * local_dims.get(0) 
                                    + node_position.get(2) * local_dims.get(0) * local_dims.get(1)
                                    ) * 27; // WARNING: supports up to a cube of ~ 880_748 by 880_748 by 880_748 nodes

                // copy the velcoity with value (0, 0, 0)  
                device_accessor_velocity_buffer_2[node_index] = device_accessor_discrete_density_buffer_1[node_index];

                // loop over all the vectors and grab the particles that will move to the current node,
                // and assign them to the associated velocity on the current node
                // also skip the velocity with value (0, 0, 0) as it does not change
                for (uint8_t i = 1; i < local_possible_velocities_count; i++)
                {
                    // get where the particles are coming from
                    // if the position to get the particles from is out of bounds it wraps around (the modulo operation)
                    uint64_t from_node_index = (
                                               ((node_position.get(0) - device_accessor_possible_velocities[i * 3]) % local_dims.get(0))
                                             + ((node_position.get(1) - device_accessor_possible_velocities[i * 3 + 1]) % local_dims.get(1)) * local_dims.get(0) 
                                             + ((node_position.get(2) - device_accessor_possible_velocities[i * 3 + 2]) % local_dims.get(2)) * local_dims.get(0) * local_dims.get(1)
                                            ) * 27;

                    // move the particles to current node with their velocity 
                    device_accessor_velocity_buffer_2[node_index + i] = device_accessor_discrete_density_buffer_1[from_node_index + i];
                }
            });
        }).wait();

        std::cout << "streaming done" << std::endl;

        // read out macroscopic variables for debugging purposes
        read_out_values(this->discrete_density_buffer_2, 27);

        std::cout << "calculating macroscopic density" << std::endl;

        // calculate per node macroscopic density
        //event macroscopic_density = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_density(*this->macro_density_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_2(*this->discrete_density_buffer_2, h);

            h.parallel_for(*this->node_count, [=](sycl::id<1> node_index) 
            {
                float node_density = 0.0f;
                for (uint8_t i = 0; i < local_possible_velocities_count; i++)
                {
                    node_density += device_accessor_discrete_density_buffer_2[node_index * local_possible_velocities_count + i];
                }
                
                device_accessor_macro_density[node_index] = node_density;
            });
        });

        std::cout << "calculating macroscopic velocity" << std::endl;

        // calculate per node macroscopic velocity
        //event macroscopic_velocity = 
        this->q.submit([&](sycl::handler& h) 
        {
            //h.depends_on(macroscopic_density);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_2(*this->discrete_density_buffer_2, h);
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_macro_density(*this->macro_density_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_x(*this->macro_velocity_x, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_y(*this->macro_velocity_y, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_z(*this->macro_velocity_z, h);

            sycl::accessor<sycl::float4, 1, sycl::access_mode::write> device_accessor_vectors(*this->vectors, h); // temp-ish copy of the macro velocity

            h.parallel_for(*this->node_count, [=](sycl::id<1> node_index) 
            {
                float macro_velocity_x = 0.0f;
                float macro_velocity_y = 0.0f;
                float macro_velocity_z = 0.0f;

                for (int i = 0; i < local_possible_velocities_count; i++)
                {
                    macro_velocity_x += device_accessor_discrete_density_buffer_2[node_index * 27 + i] * device_accessor_possible_velocities[i * 3];
                    macro_velocity_y += device_accessor_discrete_density_buffer_2[node_index * 27 + i] * device_accessor_possible_velocities[i * 3 + 1];
                    macro_velocity_z += device_accessor_discrete_density_buffer_2[node_index * 27 + i] * device_accessor_possible_velocities[i * 3 + 2];
                }

                float node_density = device_accessor_macro_density[node_index];

                macro_velocity_x = macro_velocity_x / node_density;
                macro_velocity_y = macro_velocity_y / node_density;
                macro_velocity_z = macro_velocity_z / node_density;

                device_accessor_macro_velocity_x[node_index] = macro_velocity_x;
                device_accessor_macro_velocity_y[node_index] = macro_velocity_y;
                device_accessor_macro_velocity_z[node_index] = macro_velocity_z;

                device_accessor_vectors[node_index] = sycl::float4(macro_velocity_x, macro_velocity_y, macro_velocity_z, 0.0f);
            });
        });

        // read out macroscopic variables for debugging purposes
        std::cout << "macro density" << "\n";
        read_out_values(this->macro_density_buffer, 5);
        std::cout << "macro vel" << "\n";
        read_out_values(this->macro_velocity_x, 10);
        read_out_values(this->macro_velocity_y, 10);
        read_out_values(this->macro_velocity_z, 10);

        std::cout << "starting collision..." << std::endl;

        //event collision = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);
            sycl::accessor<uint8_t, 1, sycl::access_mode::read> device_accessor_relective_index_table_new(*this->relective_index_table_new_buffer, h);
            
            sycl::accessor<uint8_t, 1, sycl::access_mode::read> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            // microscopic density 
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocity_buffer_2(*this->discrete_density_buffer_2, h);

            // macroscopic varibles
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_macro_density(*this->macro_density_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_macro_velocity_x(*this->macro_velocity_x, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_macro_velocity_y(*this->macro_velocity_y, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_macro_velocity_z(*this->macro_velocity_z, h);

            h.parallel_for(*this->discrete_density_buffer_length, [=](sycl::id<1> i) 
            {
                int local_velocity_index = i % local_possible_velocities_count;
                int node_index = i / 27;

                float weight = device_accessor_velocities_weights[local_velocity_index];

                float equlibrium_density;
                uint64_t new_index;
                
                switch (device_accessor_changeable_buffer[node_index])
                {
                case 0:
                    equlibrium_density = f_eq
                    (
                        weight, device_accessor_macro_density[node_index], // specific velocity, and the node specific density 
                        device_accessor_possible_velocities[local_velocity_index * 3],     // velocity (e_i) x val
                        device_accessor_possible_velocities[local_velocity_index * 3 + 1], // velocity (e_i) y val
                        device_accessor_possible_velocities[local_velocity_index * 3 + 2], // velocity (e_i) z val
                        device_accessor_macro_velocity_x[node_index], // node specific avg velocity
                        device_accessor_macro_velocity_y[node_index], // node specific avg velocity
                        device_accessor_macro_velocity_z[node_index]  // node specific avg velocity
                    );
                    device_accessor_discrete_density_buffer_1[i] = device_accessor_velocity_buffer_2[i] - ((device_accessor_velocity_buffer_2[i] - equlibrium_density) / local_tau);
                    break;
                
                case 1:
                    new_index = device_accessor_relective_index_table_new[local_velocity_index] + (node_index * 27);
                    device_accessor_discrete_density_buffer_1[new_index] = device_accessor_velocity_buffer_2[i];
                    break;

                case 2:
                    equlibrium_density = f_eq
                    (
                        weight, 1.0f, // specific velocity, and the node specific density 
                        device_accessor_possible_velocities[local_velocity_index * 3],     // velocity (e_i) x val
                        device_accessor_possible_velocities[local_velocity_index * 3 + 1], // velocity (e_i) y val
                        device_accessor_possible_velocities[local_velocity_index * 3 + 2], // velocity (e_i) z val
                        local_flow_vec_x, // in / out flow x velocity
                        local_flow_vec_y, // in / out flow y velocity
                        local_flow_vec_z  // in / out flow z velocity
                    );

                    device_accessor_discrete_density_buffer_1[i] = equlibrium_density;
                    break;

                default:
                    break;
                }
            });
        }).wait();

        std::cout << "collision done" << std::endl;

        read_out_values(this->discrete_density_buffer_1, 27);
        
        // copy the vectors buffer data to one of the vector arrays on the host 
        if(which_vectors_array)
        {
            this->q.submit([&](sycl::handler& h) 
            {
                sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

                h.copy(device_accessor_vectors, vectors1);

                this->vector_array = vectors1;
                this->which_vectors_array = !this->which_vectors_array;
            }).wait();
            
            this->q.submit([&](sycl::handler& h) 
            {
                sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

                h.copy(device_accessor_density, density_array_1);

                this->density_array.store(density_array_1);
            }).wait();
        }
        else
        {
            this->q.submit([&](sycl::handler& h) 
            {
                sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

                h.copy(device_accessor_vectors, vectors2);

                this->vector_array = vectors2;
                this->which_vectors_array = !this->which_vectors_array;
            }).wait();

            this->q.submit([&](sycl::handler& h) 
            {
                sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

                h.copy(device_accessor_density, density_array_2);

                this->density_array.store(density_array_2);
            }).wait();
        }

        std::cout << "copy back done" << std::endl;

        this->q.wait();
    }

    // returns a copy of the dimensions of this simulation as a 3 dimensional sycl::range object
    sycl::range<3> get_dimensions()
    {
        return sycl::range(*this->dims);
    }

    // returns the number of nodes in this simulation
    int get_node_count() 
    {
        return this->node_count->get(0);
    }
};



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

#include "float4_helper_functions.hpp" // some helper functions that act on sycl::float4 variables as 3d vectors such as the dot product
#include "buffer_debug_funcs.hpp" // some helper functions for use in debugging sycl buffers 

#include <sycl/sycl.hpp> // the main library used for parrellelism 

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

        static const uint8_t possible_velocities_number = 27;

        sycl::buffer<int8_t, 1> * possible_velocities_buffer; 


        // these are the weights that each velocity has
        // the weights add up to 1
        const float velocities_weights[27] = {
            8.0f / 27.0f, // for 1, (8 / 27)

            2.0f / 27.0f, // for 6, (2 / 27)
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 
            2.0f / 27.0f, 

            1.0f / 54.0f, // for 12, (1 / 54)
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

            1.0f / 216.0f, // for 8, (1 / 216)
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
        // 3 = sink, copies surounding cells (do no collision), set to weight
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

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // these six reference vars allow for an a-dimentioned Lattice Boltzmann Method (LBM) solver //
        // this also allows for the solver to cover more types of fluids, more easily                //
        ///////////////////////////////////////////////////////////////////////////////////////////////
        
        // reference length in meters, 
        // is also the distance between two nodes
        float ref_len; 

        // reference length in seconds 
        float ref_time;

        // reference density in ??
        float ref_density;

        // reference speed in meters per second (m/s)
        float ref_speed; 

        // the adimentional speed of sound in the lattice
        // a close approximation of 1 over the square root of 3
        static constexpr float speed_of_sound = 1.0f / 1.73205080757f; 
        
        // this changes the time it takes for the fluid to relax back to the equlibrium state
        // is related semi-directly to the fluid's viscosity 
        // a value above 0, values close to 0 become unstable
        float tau = 2.3f;

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

    // width: the width of the sim, in number of nodes
    // height: the height of the sim, in number of nodes
    // depth: the depth of the sim, in number of nodes
    // density: the density of the fluid being modeled
    // visocity: the visocity of the fluid being modeled
    // speed_of_sound: the speed of sound of the fluid being modeled in meters per second
    // node_size: the distance between each node in meters
    Simulation(int width, int height, int depth, float density, float visocity, float speed_of_sound, float node_size, float cyc_radius, float tau)
    {
        this->q = sycl::queue(sycl::gpu_selector_v);
        std::cout << "running simulation on -> " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        // reference constants for a-dimensionality 
        this->ref_len = node_size;
        this->ref_density = density;
        this->ref_time = this->ref_len / (sycl::sqrt(3.0f) * speed_of_sound);
        this->ref_speed = this->ref_len / this->ref_time;
        this->tau = (this->ref_time * visocity) / (this->ref_len * this->ref_len * this->ref_speed * this->ref_speed) + 0.5f;

        this->tau = tau;


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

        // initalize the descrete density buffer at their respective weights
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);

            h.parallel_for(*this->discrete_density_buffer_length, [=](sycl::id<1> i) 
            {
                device_accessor_discrete_density_buffer_1[i] = device_accessor_velocities_weights[i % 27];
            });
        }).wait();

        // add a bit of random noise to it
        {
            auto accessor = discrete_density_buffer_1->get_host_access();
            for (uint64_t i = 0; i < discrete_density_buffer_length->get(0); i++)
            {
                accessor[i] += (rand() % 100) / 1000.0f; // + 0.00, 0.01, 0.02, to 0.99f
            }  
        }
        
        // set which nodes are boundary nodes
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<uint8_t, 1, sycl::access_mode::write> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            // currently a cylinder aligned along the y axis with radius r with the center at (width / 2), y, (depth / 6)
            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                int64_t index = i.get(0) + i.get(1) * width + i.get(2) * width * height;

                device_accessor_changeable_buffer[index] = 0;

                float r_square = cyc_radius * cyc_radius;

                float x = i.get(0) - (width / 2.0f);
                float z = i.get(2) - (depth / 6.0f);

                if( x*x + z*z < r_square )
                {
                    device_accessor_changeable_buffer[index] = 1;
                }

                if(i.get(0) == 0 || i.get(0) == width - 1)
                {
                    // device_accessor_changeable_buffer[index] = 1;
                }
                
                if(i.get(2) == 0)
                {
                    device_accessor_changeable_buffer[index] = 2;
                }
                if(i.get(2) == depth - 1)
                {
                    device_accessor_changeable_buffer[index] = 3;
                }
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

        // make sure all jobs are complete
        q.wait();
    }

    // read-only access to the main density buffer,
    // useful for debugging and/or networking, as it will block any other job on/access to this buffer from running until the accessor is freed
    sycl::host_accessor<float, 1, sycl::access_mode::read> get_accessor_for_discrete_density_buffer_1()
    {
        return this->discrete_density_buffer_1->get_host_access();
    }
    
    // read-only access to the changable buffer,
    // useful for debugging and/or networking, as it will block any other job on/access to this buffer from running until the accessor is freed
    sycl::host_accessor<uint8_t, 1, sycl::access_mode::read> get_accessor_for_changeable_buffer()
    {
        return this->changeable_buffer->get_host_access();
    }

    /**
     * calculate the next state of the simulation using the values given
     * moving the sim to the next time with the calculated timestep (new_time = current + ref_time)
     */
    void next_frame()
    {
        int local_possible_velocities_count = this->possible_velocities_number;

        sycl::range<3> local_dims = *this->dims;
        sycl::range<1> local_velocity_buffer_length = *this->discrete_density_buffer_length;

        float local_tau = this->tau;

        float local_flow_vec_x = this->flow_vec_x;
        float local_flow_vec_y = this->flow_vec_y;
        float local_flow_vec_z = this->flow_vec_z;

        sycl::event compute_streaming = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);

            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_discrete_density_buffer_2(*this->discrete_density_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> node_position) 
            {
                int node_x = node_position.get(0);
                int node_y = node_position.get(1);
                int node_z = node_position.get(2);
                // WARNING -> a 64 bit integer supports up to a cube of ~ 880_748 by 880_748 by 880_748 nodes
                //                  = x + y * width + z * width * height;
                uint64_t node_index = (
                                      node_x                                  
                                    + node_y * local_dims.get(0) 
                                    + node_z * local_dims.get(0) * local_dims.get(1)
                                    ) * 27; 

                // copy the velocity with value (0, 0, 0)  
                device_accessor_discrete_density_buffer_2[node_index] = device_accessor_discrete_density_buffer_1[node_index];

                int from_node_x;
                int from_node_y;
                int from_node_z;

                // loop over all the rest of the vectors and grab the particles that will move to the current node,
                // and assign them to the associated velocity on the current node
                for (uint8_t i = 1; i < local_possible_velocities_count; i++)
                {
                    from_node_x = node_x - device_accessor_possible_velocities[i * 3];
                    from_node_y = node_y - device_accessor_possible_velocities[i * 3 + 1];
                    from_node_z = node_z - device_accessor_possible_velocities[i * 3 + 2];

                    // do these checks to make sure the from node is in bounds, 
                    // can't use the modulus operator   
                    from_node_x = from_node_x < 0 ? local_dims.get(0) - 1 : from_node_x;
                    from_node_y = from_node_y < 0 ? local_dims.get(1) - 1 : from_node_y;
                    from_node_z = from_node_z < 0 ? local_dims.get(2) - 1 : from_node_z;

                    from_node_x = from_node_x > (local_dims.get(0) - 1) ? 0 : from_node_x;
                    from_node_y = from_node_y > (local_dims.get(1) - 1) ? 0 : from_node_y;
                    from_node_z = from_node_z > (local_dims.get(2) - 1) ? 0 : from_node_z;

                    // get where the particles are coming from
                    // if the position to get the particles from is out of bounds it wraps around (the modulo operation)
                    uint64_t from_node_index = (
                                               from_node_x
                                             + from_node_y * local_dims.get(0) 
                                             + from_node_z * local_dims.get(0) * local_dims.get(1)
                                            ) * 27;

                    // move the particles to current node with their velocity 
                    device_accessor_discrete_density_buffer_2[node_index + i] = device_accessor_discrete_density_buffer_1[from_node_index + i];
                }
            });
        });

        sycl::event compute_macroscopic_variables = 
        this->q.submit([&](sycl::handler& h) 
        {
            h.depends_on(compute_streaming);
            
            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_2(*this->discrete_density_buffer_2, h);
            
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_density(*this->macro_density_buffer, h);
            
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_x(*this->macro_velocity_x, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_y(*this->macro_velocity_y, h);
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_macro_velocity_z(*this->macro_velocity_z, h);

            sycl::accessor<sycl::float4, 1, sycl::access_mode::write> device_accessor_vectors(*this->vectors, h); // temp-ish copy of the macro velocity


            h.parallel_for(*this->node_count, [=](sycl::id<1> node_index) 
            {
                float node_density = 0.0f;
                
                float macro_velocity_x = 0.0f;
                float macro_velocity_y = 0.0f;
                float macro_velocity_z = 0.0f;

                for (uint8_t i = 0; i < local_possible_velocities_count; i++)
                {
                    float density = device_accessor_discrete_density_buffer_2[node_index * local_possible_velocities_count + i];

                    node_density += sycl::fabs(density); // absoulute value of density

                    macro_velocity_x += density * device_accessor_possible_velocities[i * 3];
                    macro_velocity_y += density * device_accessor_possible_velocities[i * 3 + 1];
                    macro_velocity_z += density * device_accessor_possible_velocities[i * 3 + 2];
                }

                macro_velocity_x /= node_density;
                macro_velocity_y /= node_density;
                macro_velocity_z /= node_density;
                
                float macro_velocity_len = sycl::sqrt(macro_velocity_x * macro_velocity_x + macro_velocity_y * macro_velocity_y + macro_velocity_z * macro_velocity_z);
                if(macro_velocity_len > speed_of_sound)
                {
                    macro_velocity_x = (macro_velocity_x / macro_velocity_len) * speed_of_sound;
                    macro_velocity_y = (macro_velocity_y / macro_velocity_len) * speed_of_sound;
                    macro_velocity_z = (macro_velocity_z / macro_velocity_len) * speed_of_sound;
                }

                device_accessor_macro_velocity_x[node_index] = macro_velocity_x;
                device_accessor_macro_velocity_y[node_index] = macro_velocity_y;
                device_accessor_macro_velocity_z[node_index] = macro_velocity_z;

                device_accessor_vectors[node_index] = sycl::float4(macro_velocity_x, macro_velocity_y, macro_velocity_z, 0.0f);
                
                device_accessor_macro_density[node_index] = node_density;
            });
        });


        sycl::event compute_collision = 
        this->q.submit([&](sycl::handler& h) 
        {
            h.depends_on(compute_macroscopic_variables);

            sycl::accessor<int8_t, 1, sycl::access_mode::read> device_accessor_possible_velocities(*this->possible_velocities_buffer, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_velocities_weights(*this->velocities_weights_buffer, h);
            sycl::accessor<uint8_t, 1, sycl::access_mode::read> device_accessor_relective_index_table_new(*this->relective_index_table_new_buffer, h);
            
            sycl::accessor<uint8_t, 1, sycl::access_mode::read> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            // microscopic density 
            sycl::accessor<float, 1, sycl::access_mode::write> device_accessor_discrete_density_buffer_1(*this->discrete_density_buffer_1, h);
            sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_discrete_density_buffer_2(*this->discrete_density_buffer_2, h);

            // macroscopic variables
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
                    device_accessor_discrete_density_buffer_1[i] = device_accessor_discrete_density_buffer_2[i] - (local_tau * (device_accessor_discrete_density_buffer_2[i] - equlibrium_density));
                    break;
                
                case 1:
                    new_index = device_accessor_relective_index_table_new[local_velocity_index] + (node_index * 27);
                    device_accessor_discrete_density_buffer_1[new_index] = device_accessor_discrete_density_buffer_2[i];
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

                case 3:
                    device_accessor_discrete_density_buffer_1[i] = weight;
                break;

                default:
                    break;
                }
            });
        });
        
        // copy the vectors buffer data to one of the vector arrays on the host 
        if(which_vectors_array)
        {
            this->q.submit([&](sycl::handler& h) 
            {
                h.depends_on(compute_macroscopic_variables);

                sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

                h.copy(device_accessor_vectors, vectors1);

                this->vector_array = vectors1;
                this->which_vectors_array = !this->which_vectors_array;
            });
            
            this->q.submit([&](sycl::handler& h) 
            {
                h.depends_on(compute_macroscopic_variables);

                sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

                h.copy(device_accessor_density, density_array_1);

                this->density_array.store(density_array_1);
            });
        }
        else
        {
            this->q.submit([&](sycl::handler& h) 
            {
                h.depends_on(compute_macroscopic_variables);

                sycl::accessor<sycl::float4, 1, sycl::access_mode::read> device_accessor_vectors(*this->vectors, h);

                h.copy(device_accessor_vectors, vectors2);

                this->vector_array = vectors2;
                this->which_vectors_array = !this->which_vectors_array;
            });

            this->q.submit([&](sycl::handler& h) 
            {
                h.depends_on(compute_macroscopic_variables);

                sycl::accessor<float, 1, sycl::access_mode::read> device_accessor_density(*this->macro_density_buffer, h);

                h.copy(device_accessor_density, density_array_2);

                this->density_array.store(density_array_2);
            });
        }

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



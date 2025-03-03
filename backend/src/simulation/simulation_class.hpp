/*
    name: simulation_class.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for a custom simulation class to model incompressible fluids and gas
*/ 
#include <iostream>
#include <atomic>

#include "float4_helper_functions.hpp" // a few helper functions such as dot product, magnitude etc. for the xyz componenets of a float4
#include "sim_setup_helper_functions.hpp" // a few helper functional objects to setup the sim in various configurations

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>

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

/**
 * this simulation uses the lattice boltzmann method of computational fluid dynamics
 * 
 * the simulation is made up of 
 *      a. nodes (points) in 3s space
 *      b. and of i number of velocities, that are the speeds that the particles at a particluar node can have 
 * 
 * the simulation works in two steps that are done for each call of the next_frame function 
 * 
 * 1. advection
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
        int arrLen;

        sycl::queue q;

        /**
         * velocity x,y,z 
         * and ability of that velocity to change in the w component 
         *      (0.0f to 1.0f) 0 meaning can't change (aka a wall) 1 meaning it can change to the fullest extent
         */

        // pointers to the buffers (memory objects)
        sycl::buffer<bool,   3> * changeable_buffer;
        sycl::buffer<sycl::float4, 3> * velocity_vectors_buffer_1;
        sycl::buffer<sycl::float4, 3> * velocity_vectors_buffer_2;

        // the three dimensional dimensions of the simulation, 
        // the real size of one dimension length is the length of that dimension times the ref_len  
        sycl::range<3> * dims;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // these four reference vars allow for an a-dimentioned Lattice Boltzmann Method (LBM) solver //
        // this also allows for the solver to cover more types of fluids, more easily                 //
        ////////////////////////////////////////////////////////////////////////////////////////////////

        // reference length in meters, 
        // is also the distance between two nodes
        // and the width, height, depth of a node
        float ref_len; 

        // reference length in seconds 
        float ref_time; 

        // reference density in ??
        float ref_density; 

        // speed in meters per second (m/s)
        float ref_speed = ref_time / ref_density; 

    public:
        const int* const pWidth = &width;
        const int* const pHeight = &height;
        const int* const pDepth = &depth;

        const int* const pAmtOfCells = &arrLen;
        const int* const pVelocityArrLen = &arrLen;

        // x and y and z velocity vector components + w for density
        sycl::float4* vectors;

        /**
         * one per node, the ability of the node to change,
         * true means the node is not part of a solid object, false means it is part of empty space (aka the fluid can move freely)
         */ 
        bool* changeable;

    //constructor
    Simulation(int width, int height, int depth)
    {
        this->q = sycl::queue();
        std::cout << "running simulation on -> " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        this->arrLen = this->width * this->height * this->depth;

        this->dims = new sycl::range<3>(width, height, depth);

        this->changeable_buffer         = new sycl::buffer<bool,   3>(*this->dims);
        this->velocity_vectors_buffer_1 = new sycl::buffer<sycl::float4, 3>(*this->dims);
        this->velocity_vectors_buffer_2 = new sycl::buffer<sycl::float4, 3>(*this->dims);

        this->q.submit([&](sycl::handler& h) 
        {
            auto func = CyclinderInBox(
                sycl::accessor<bool, 3, sycl::access_mode::write>(*this->changeable_buffer, h),
                sycl::accessor<sycl::float4, 3, sycl::access_mode::write>(*this->velocity_vectors_buffer_1, h),
                *this->dims,
                2.0f
            );

            h.parallel_for(*this->dims, func);
        }).wait();
    }

    // deconstructor
    ~Simulation()
    {
        // free the allocated memory in the device
        free(this->changeable_buffer, q); 
        free(this->velocity_vectors_buffer_1, q); 
        free(this->velocity_vectors_buffer_2, q); 

        free(this->dims);
    }

    void next_frame(float dt)
    {
        std::cout << "starting done" << std::endl;

        // do the math
        //event modifyVelocityValues = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                device_accessor_velocity_buffer_1[i].y() -= 1.0f * dt;

                int width;
                int height;
                int depth;
                
                if(i.get(0) == 0 || i.get(0) == width - 1) 
                {
                    device_accessor_velocity_buffer_1[i].x() = 1.0f;
                }
            });
        }).wait();

        std::cout << "modifyVelocityValues done" << std::endl;
        //event projection = 
        this->q.submit([&](sycl::handler& h) 
        {
            sycl::accessor<bool, 3, sycl::access_mode::read> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            sycl::accessor<sycl::float4, 3, sycl::access_mode::read> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);
            //h.depends_on(modifyVelocityValues);

            /**
             * this is an idea that is being testing 
             * taking the projection method from the 10 minute physics video: https://www.youtube.com/watch?v=iKAVRgIrUOU&t=27s
             * where there are velocities at the edges of 2d squares and the fluid moving in and out of the squares has to be equal
             * and applying it to a 3d grid of cells where the velocity components are at the center.
             * 
             * by taking every in between cube of 8 velocities and making their velocities match so that the sum of their vectors results in a vector of zero length, 
             * meaning no fluid is moving in or out. 
             * 
             * https://www.desmos.com/calculator/z7quq4qock a graph of a single 2d cell and the math required to balence it  
             */
            sycl::range temp(this->width - 1, this->height - 1, this->depth - 1);
            h.parallel_for(temp, [=](sycl::id<3> i) 
            { 
                int width;
                int height;
                int depth;

                float overRelax;

                // x y z
                
                // x+1 y z 
                // x y+1 z
                // x y z+1

                // x y+1 z+1
                // x+1 y z+1
                // x+1 y+1 z

                // x+1 y+1 z+1

                /*
                 *     5 ______8
                 *     /|     /|
                 *    / |    / |
                 *  3/__|__7/  |
                 *   |  |__|___|
                 *   | 4/  |  /6
                 *   | /   | /
                 *   |/____|/
                 *   1     2
                 */

                sycl::float4 velocity1 = device_accessor_velocity_buffer_1[i.get(0)    ][i.get(1)    ][i.get(2)    ];
                sycl::float4 velocity2 = device_accessor_velocity_buffer_1[i.get(0) + 1][i.get(1)    ][i.get(2)    ];
                sycl::float4 velocity3 = device_accessor_velocity_buffer_1[i.get(0)    ][i.get(1) + 1][i.get(2)    ];
                sycl::float4 velocity4 = device_accessor_velocity_buffer_1[i.get(0)    ][i.get(1)    ][i.get(2) + 1];
                sycl::float4 velocity5 = device_accessor_velocity_buffer_1[i.get(0)    ][i.get(1) + 1][i.get(2) + 1];
                sycl::float4 velocity6 = device_accessor_velocity_buffer_1[i.get(0) + 1][i.get(1)    ][i.get(2) + 1];
                sycl::float4 velocity7 = device_accessor_velocity_buffer_1[i.get(0) + 1][i.get(1) + 1][i.get(2)    ];
                sycl::float4 velocity8 = device_accessor_velocity_buffer_1[i.get(0) + 1][i.get(1) + 1][i.get(2) + 1];

                bool vel1change = device_accessor_changeable_buffer[i.get(0)    ][i.get(1)    ][i.get(2)    ];
                bool vel2change = device_accessor_changeable_buffer[i.get(0) + 1][i.get(1)    ][i.get(2)    ];
                bool vel3change = device_accessor_changeable_buffer[i.get(0)    ][i.get(1) + 1][i.get(2)    ];
                bool vel4change = device_accessor_changeable_buffer[i.get(0)    ][i.get(1)    ][i.get(2) + 1];
                bool vel5change = device_accessor_changeable_buffer[i.get(0)    ][i.get(1) + 1][i.get(2) + 1];
                bool vel6change = device_accessor_changeable_buffer[i.get(0) + 1][i.get(1)    ][i.get(2) + 1];
                bool vel7change = device_accessor_changeable_buffer[i.get(0) + 1][i.get(1) + 1][i.get(2)    ];
                bool vel8change = device_accessor_changeable_buffer[i.get(0) + 1][i.get(1) + 1][i.get(2) + 1];

                if( !vel1change &&
                    !vel2change &&
                    !vel3change &&
                    !vel4change &&
                    !vel5change &&
                    !vel6change &&
                    !vel7change &&
                    !vel8change ) 
                    {
                        return;
                    }

                sycl::float4 velocity1projection = Projection3d(velocity1, -1, -1, -1);
                sycl::float4 velocity2projection = Projection3d(velocity1,  1, -1, -1);
                sycl::float4 velocity3projection = Projection3d(velocity1, -1,  1, -1);
                sycl::float4 velocity4projection = Projection3d(velocity1, -1, -1,  1);
                sycl::float4 velocity5projection = Projection3d(velocity1, -1,  1,  1);
                sycl::float4 velocity6projection = Projection3d(velocity1,  1, -1,  1);
                sycl::float4 velocity7projection = Projection3d(velocity1,  1,  1, -1);
                sycl::float4 velocity8projection = Projection3d(velocity1,  1,  1,  1);

                // the projection of magnitude of the vector u when vector u is projected on to vector v is:
                // (vector u dotproduct vector v) divided by the magnitude of vector v

                // magnitude of the vectors projected onto a vector going through the middle of the 8 positions, and the nth position
                float out1 = magnitude(velocity1projection);
                float out2 = magnitude(velocity2projection);
                float out3 = magnitude(velocity3projection);
                float out4 = magnitude(velocity4projection);
                float out5 = magnitude(velocity5projection);
                float out6 = magnitude(velocity6projection);
                float out7 = magnitude(velocity7projection);
                float out8 = magnitude(velocity8projection);

                // // not equal to but proportional to the actual amount of outflow
                float divergence = out1 + out2 + out3 + out4 + out5 + out6 + out7 + out8;
                divergence = divergence * overRelax;

                float change_amount = 1.0f/((int) vel1change + 
                                            (int) vel2change + 
                                            (int) vel3change + 
                                            (int) vel4change + 
                                            (int) vel5change + 
                                            (int) vel6change + 
                                            (int) vel7change + 
                                            (int) vel8change );

                float velocity1change = vel1change * change_amount;
                float velocity2change = vel2change * change_amount;
                float velocity3change = vel3change * change_amount;
                float velocity4change = vel4change * change_amount;
                float velocity5change = vel5change * change_amount;
                float velocity6change = vel6change * change_amount;
                float velocity7change = vel7change * change_amount;
                float velocity8change = vel8change * change_amount;
                
                device_accessor_velocity_buffer_2[i.get(0)    ][i.get(1)    ][i.get(2)    ] = velocity1 + (velocity1projection - scale3d(velocity1projection, divergence * velocity1change));
                device_accessor_velocity_buffer_2[i.get(0) + 1][i.get(1)    ][i.get(2)    ] = velocity2 + (velocity2projection - scale3d(velocity2projection, divergence * velocity2change));
                device_accessor_velocity_buffer_2[i.get(0)    ][i.get(1) + 1][i.get(2)    ] = velocity3 + (velocity3projection - scale3d(velocity3projection, divergence * velocity3change));
                device_accessor_velocity_buffer_2[i.get(0)    ][i.get(1)    ][i.get(2) + 1] = velocity4 + (velocity4projection - scale3d(velocity4projection, divergence * velocity4change));
                device_accessor_velocity_buffer_2[i.get(0)    ][i.get(1) + 1][i.get(2) + 1] = velocity5 + (velocity5projection - scale3d(velocity5projection, divergence * velocity5change));
                device_accessor_velocity_buffer_2[i.get(0) + 1][i.get(1)    ][i.get(2) + 1] = velocity6 + (velocity6projection - scale3d(velocity6projection, divergence * velocity6change));
                device_accessor_velocity_buffer_2[i.get(0) + 1][i.get(1) + 1][i.get(2)    ] = velocity7 + (velocity7projection - scale3d(velocity7projection, divergence * velocity7change));
                device_accessor_velocity_buffer_2[i.get(0) + 1][i.get(1) + 1][i.get(2) + 1] = velocity8 + (velocity8projection - scale3d(velocity8projection, divergence * velocity8change));
            });
        }).wait();
        std::cout << "projection done" << std::endl;
        

        //event advection = 
        this->q.submit([&](sycl::handler& h) 
        {
            //h.depends_on(projection);
            sycl::accessor<bool, 3, sycl::access_mode::read> device_accessor_changeable_buffer(*this->changeable_buffer, h);

            sycl::accessor<sycl::float4, 3, sycl::access_mode::read> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                if(device_accessor_changeable_buffer[i] == false)
                {
                    return;
                }

                int width;
                int height;
                int depth;

                float previousX = i.get(0) - (device_accessor_velocity_buffer_1[i].x() * dt); // position to grab values from, over/under flow positions wrap around 
                float previousY = i.get(1) - (device_accessor_velocity_buffer_1[i].y() * dt); 
                float previousZ = i.get(2) - (device_accessor_velocity_buffer_1[i].z() * dt);

                // previousX = previousX - (width - 1)  * sycl::_V1::floor(previousX / (width - 1)); // mod so that they are within the range: (0 to width - 1)
                // previousY = previousY - (height - 1) * sycl::_V1::floor(previousY / (height - 1)); // (0 to height)
                // previousZ = previousZ - (depth - 1)  * sycl::_V1::floor(previousZ / (depth - 1)); // (0 to depth)

                previousX = sycl::_V1::fmin(previousX, 0);
                previousY = sycl::_V1::fmin(previousY, 0);
                previousZ = sycl::_V1::fmin(previousZ, 0);

                previousX = sycl::_V1::fmax(previousX, width - 1);
                previousY = sycl::_V1::fmax(previousY, height - 1);
                previousZ = sycl::_V1::fmax(previousZ, depth - 1);

                //calc the x y z linear interpolation components seperately 
                int maxX = sycl::_V1::ceil(previousX);
                int maxY = sycl::_V1::ceil(previousY);
                int maxZ = sycl::_V1::ceil(previousZ);
                int minX = sycl::_V1::floor(previousX);
                int minY = sycl::_V1::floor(previousY);
                int minZ = sycl::_V1::floor(previousZ);

                float xDec = previousX - sycl::_V1::floor(previousX); // decimal component of previousX
                float yDec = previousY - sycl::_V1::floor(previousY); // decimal component of previousY
                float zDec = previousZ - sycl::_V1::floor(previousZ); // decimal component of previousZ

                //trilinear interpolated velocity value
                device_accessor_velocity_buffer_2[i] = (
                (
                (device_accessor_velocity_buffer_1[minX][minY][minZ] * (yDec) + device_accessor_velocity_buffer_1[minX][maxY][minZ] * (1-yDec)) * (xDec) +
                (device_accessor_velocity_buffer_1[maxX][minY][minZ] * (yDec) + device_accessor_velocity_buffer_1[maxX][maxY][minZ] * (1-yDec)) * (1-xDec)
                ) 
                * (zDec) +
                (
                (device_accessor_velocity_buffer_1[minX][minY][maxZ] * (yDec) + device_accessor_velocity_buffer_1[minX][maxY][maxZ] * (1-yDec)) * (xDec) + 
                (device_accessor_velocity_buffer_1[maxX][minY][maxZ] * (yDec) + device_accessor_velocity_buffer_1[maxX][maxY][maxZ] * (1-yDec)) * (1-xDec)
                ) 
                * (1-zDec));

                //out << i << ": " << velocityArray[i].x() << " " << velocityArray[i].y() << " " << velocityArray[i].z() << " " << velocityArray[i].w() << " | ";
            });
        }).wait(); 
        std::cout << "advection done" << std::endl;


        // then copy it back to the vectors host array
        this->q.submit([&](sycl::handler& h) 
        {
            //h.depends_on(advection);

            sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_1(*this->velocity_vectors_buffer_1, h);
            sycl::accessor<sycl::float4, 3, sycl::access_mode::read> device_accessor_velocity_buffer_2(*this->velocity_vectors_buffer_2, h);

            h.parallel_for(*this->dims, [=](sycl::id<3> i) 
            {
                device_accessor_velocity_buffer_1[i] = device_accessor_velocity_buffer_2[i];
            });
        });
        std::cout << "copy done" << std::endl;

        this->q.wait();
    }
};

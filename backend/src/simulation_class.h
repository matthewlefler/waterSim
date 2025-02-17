/*
    name: simulation_class.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for a custom simulation class to model incompressible fluids and gas
*/ 

#include <vector>
#include <iostream>
#include <atomic>
#include <array>

#include "float4_helper_functions.h" // a few helper functions such as dot product, magnitude etc. for the xyz componenets of a float4

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>
using namespace sycl;

//<-- TEMPERARY DEBUGGING FUNCTION !-->
// Our simple asynchronous handler function
auto handle_async_error = [](exception_list elist) {
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

// TODO: define spacing between cells -> distance per second vs current cells per second
// TODO: make proper edges -> add .w() component consideration to projection step

/**
 * the simulation is made up of 3d cells (or voxels if you want to think of it that way) that has a static size
 * the simulation works in three steps that are done for each call of the next_frame function 
 * 
 * 1. modify velocity values
 *      add any external forces such as gravity 
 *      and update the w component as any objects change positions etc.
 * 2. projection 
 *      makes the fluid incompressible, 
 *      the velocities moving in and out of a cell are modified
 *      to make the overall amount of fluid moving in and out equal
 * 3. advection
 *      move the velocity field
 *      modifies the velocities again to mimic particles carring the velocities
 * 
 * each cell has a "density" value in the middle of the cell; length of this array = width * height * depth
 *
 * and there is 1 velocities/faces per cell
 * 
 * they are stored, given a position x,y,z:
 * in major row order, x increases then y then z 
 * 
 */
class Simulation
{
    private:
        int width;
        int height;
        int depth;
        int arrLen;

        queue q;

        /**
         * velocity x,y,z 
         * and ability of that velocity to change in the w component 
         *      (0.0f to 1.0f) 0 meaning can't change (aka a wall) 1 meaning it can change to the fullest extent
         */

        // the divergence of the velocities of a cell are mutiplied by this, it helps to keep the simulation from collapsing
        const float overRelax = 1.0f;

        sycl::buffer<bool, 3> changeableBuffer;
        sycl::buffer<float4, 3> velocityVectorsBuffer1;
        sycl::buffer<float4, 3> velocityVectorsBuffer2;

    public:
        const int* const pWidth = &width;
        const int* const pHeight = &height;
        const int* const pDepth = &depth;

        const int* const pAmtOfCells = &arrLen;
        const int* const pVelocityArrLen = &arrLen;

        // x and y and z velocity vector components + w for density
        float4* vectors;
        // one per velocity, the ablility of the velocity to change, true is yes false is no
        bool* changeable;

    //constructor
    Simulation(int width, int height, int depth)
    {
        this->q = queue();
        std::cout << "running simulation on -> " << q.get_device().get_info<info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        this->arrLen = this->width * this->height * this->depth;



        changeableBuffer = new buffer(changeable, );

        // copy vectors to the device
        this->send();
    }

    // deconstructor
    ~Simulation()
    {
        // free the allocated memory in the device
        free(this->changeableBuffer, q); 
        free(this->velocityVectorsBuffer, q); 
        free(this->deviceOverRelax, q);

        delete[] this->velocityVectors; // free the array memory
        delete[] this->changeableArr; // x2
    }

    /**
     * recopy the changeability, velocity arrays, and the width, height, depth, and overrelax to the device attached to the queue,
     * overwriting the data already there
     */
    void send()
    {
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceVelocityArr, this->velocityVectors, this->arrLen * sizeof(float4));
        });

        // copy changability of velocities to the device
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceChangeableArr, this->changeableArr, this->arrLen * sizeof(bool));
        });

        // copy depth variable to device
        this->q.submit([&](handler& h) { 
            h.memcpy(this->deviceWidth, &this->width, sizeof(int));
        });

        // copy width variable to device
        this->q.submit([&](handler& h) { 
            h.memcpy(this->deviceHeight, &this->height, sizeof(int));
        });

        // copy height variable to device
        this->q.submit([&](handler& h) { 
            h.memcpy(this->deviceDepth, &this->depth, sizeof(int));
        });

        // copy overrelax variable to device
        this->q.submit([&](handler& h) { 
            h.memcpy(this->deviceOverRelax, &this->overRelax, sizeof(float));
        });

        q.wait(); // wait for all operations to complete
    }

    void next_frame(float dt)
    {
        float4* velocityArray = this->deviceVelocityArr;
        bool* changeableArray = this->deviceChangeableArr;
                
        int* pWidth = deviceWidth;
        int* pHeight = deviceHeight;
        int* pDepth = deviceDepth;

        float* pOverRelax = deviceOverRelax;
        
        std::cout << "starting done" << std::endl;

        // do the math
        //event modifyVelocityValues = 
        this->q.submit([&](handler& h) 
        {
            h.parallel_for(this->arrLen, [=](id<1> i) 
            {
                velocityArray[i].y() -= 1.0f * dt;

                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                int x = i % (width - 1);
                int y = (i / (width - 1)) % (height - 1);
                int z = i / ((width - 1) * (height - 1));
                
                if(x == 0 || x == width - 1) 
                {
                    velocityArray[i].x() = 1.0f;
                }
            });
        }).wait();

        std::cout << "modifyVelocityValues done" << std::endl;
        //event projection = 
        this->q.submit([&](handler& h) 
        {
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
            h.parallel_for((this->width - 1) * (this->height - 1) * (this->depth - 1), [=](id<1> i) 
            { 
                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                float overRelax = *pOverRelax;

                
                int x = 1;//i % (width - 1);
                int y = 1;//(i / (width - 1)) % (height - 1);
                int z = 1;//i / ((width - 1) * (height - 1)); 

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

                float4 velocity1 = velocityArray[x     + (y       * height) + (z       * width * height)];
                float4 velocity2 = velocityArray[x + 1 + (y       * height) + (z       * width * height)];
                float4 velocity3 = velocityArray[x     + ((y + 1) * height) + (z       * width * height)];
                float4 velocity4 = velocityArray[x     + (y       * height) + ((z + 1) * width * height)];
                float4 velocity5 = velocityArray[x     + ((y + 1) * height) + ((z + 1) * width * height)];
                float4 velocity6 = velocityArray[x + 1 + (y       * height) + ((z + 1) * width * height)];
                float4 velocity7 = velocityArray[x + 1 + ((y + 1) * height) + (z       * width * height)];
                float4 velocity8 = velocityArray[x + 1 + ((y + 1) * height) + ((z + 1) * width * height)];

                bool vel1change = changeableArray[x     + (y       * height) + (z       * width * height)];
                bool vel2change = changeableArray[x + 1 + (y       * height) + (z       * width * height)];
                bool vel3change = changeableArray[x     + ((y + 1) * height) + (z       * width * height)];
                bool vel4change = changeableArray[x     + (y       * height) + ((z + 1) * width * height)];
                bool vel5change = changeableArray[x     + ((y + 1) * height) + ((z + 1) * width * height)];
                bool vel6change = changeableArray[x + 1 + (y       * height) + ((z + 1) * width * height)];
                bool vel7change = changeableArray[x + 1 + ((y + 1) * height) + (z       * width * height)];
                bool vel8change = changeableArray[x + 1 + ((y + 1) * height) + ((z + 1) * width * height)];


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

                float4 velocity1projection = Projection3d(velocity1, -1, -1, -1);
                float4 velocity2projection = Projection3d(velocity1,  1, -1, -1);
                float4 velocity3projection = Projection3d(velocity1, -1,  1, -1);
                float4 velocity4projection = Projection3d(velocity1, -1, -1,  1);
                float4 velocity5projection = Projection3d(velocity1, -1,  1,  1);
                float4 velocity6projection = Projection3d(velocity1,  1, -1,  1);
                float4 velocity7projection = Projection3d(velocity1,  1,  1, -1);
                float4 velocity8projection = Projection3d(velocity1,  1,  1,  1);

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
                
                velocityArray[x     + (y       * height) + (z       * width * height)] = velocity1 + (velocity1projection - scale3d(velocity1projection, divergence * velocity1change));
                velocityArray[x + 1 + (y       * height) + (z       * width * height)] = velocity2 + (velocity2projection - scale3d(velocity2projection, divergence * velocity2change));
                velocityArray[x     + ((y + 1) * height) + (z       * width * height)] = velocity3 + (velocity3projection - scale3d(velocity3projection, divergence * velocity3change));
                velocityArray[x     + (y       * height) + ((z + 1) * width * height)] = velocity4 + (velocity4projection - scale3d(velocity4projection, divergence * velocity4change));
                velocityArray[x     + ((y + 1) * height) + ((z + 1) * width * height)] = velocity5 + (velocity5projection - scale3d(velocity5projection, divergence * velocity5change));
                velocityArray[x + 1 + (y       * height) + ((z + 1) * width * height)] = velocity6 + (velocity6projection - scale3d(velocity6projection, divergence * velocity6change));
                velocityArray[x + 1 + ((y + 1) * height) + (z       * width * height)] = velocity7 + (velocity7projection - scale3d(velocity7projection, divergence * velocity7change));
                velocityArray[x + 1 + ((y + 1) * height) + ((z + 1) * width * height)] = velocity8 + (velocity8projection - scale3d(velocity8projection, divergence * velocity8change));
            });
        }).wait();
        std::cout << "projection done" << std::endl;
        

        //event advection = 
        this->q.submit([&](handler& h) 
        {
            //h.depends_on(projection);

            h.parallel_for(this->arrLen, [=](id<1> i) 
            {
                if(changeableArray[i] == false)
                {
                    return;
                }

                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                int x = 1;//i % width;
                int y = 1;//(i / width) % height;
                int z = 1;//i / (width * height);

                float previousX = x - (velocityArray[i].x() * dt); // position to grab values from, over/under flow positions wrap around 
                float previousY = y - (velocityArray[i].y() * dt); 
                float previousZ = z - (velocityArray[i].z() * dt);

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
                velocityArray[i] = (
                (
                (velocityArray[minX + minY * height + minZ * width * height] * (yDec) + velocityArray[minX + maxY * height + minZ * width * height] * (1-yDec)) * (xDec) +
                (velocityArray[maxX + minY * height + minZ * width * height] * (yDec) + velocityArray[maxX + maxY * height + minZ * width * height] * (1-yDec)) * (1-xDec)
                ) 
                * (zDec) +
                (
                (velocityArray[minX + minY * height + maxZ * width * height] * (yDec) + velocityArray[minX + maxY * height + maxZ * width * height] * (1-yDec)) * (xDec) + 
                (velocityArray[maxX + minY * height + maxZ * width * height] * (yDec) + velocityArray[maxX + maxY * height + maxZ * width * height] * (1-yDec)) * (1-xDec)
                ) 
                * (1-zDec));

                //out << i << ": " << velocityArray[i].x() << " " << velocityArray[i].y() << " " << velocityArray[i].z() << " " << velocityArray[i].w() << " | ";
            });
        }).wait(); 
        std::cout << "advection done" << std::endl;


        // then copy it back to the vectors host array
        this->q.submit([&](handler& h) 
        {
            //h.depends_on(advection);

            h.memcpy(this->velocityVectors, deviceVelocityArr, this->arrLen * sizeof(float4));
        });
        std::cout << "copy done" << std::endl;


        this->q.wait();
    }

    // TODO: fix this function somehow
    void printVelocities()
    {
        std::cout << "sim print: "; 
        for (int i = 0; i < this->arrLen; ++i)
        {
            float4 vec = (*(&this->vectors))[i];
            std::cout << " | v" << i << ": " << vec.x() << " " << vec.y() << " " << vec.z() << " " << vec.w() << " ";
        }
        std::cout << "\n";
    }

    inline int getXPos(int index)
    {
        return index % this->width;
    }
    inline int getYPos(int index)
    {
        return (index / this->width) % this->height;
    }
    inline int getZPos(int index)
    {
        return index / (this->width * this->height);
    }

    inline int getIndex(int x, int y, int z)
    {
        return x + (y * this->height) + (z * this->width * this->height);
    }
};

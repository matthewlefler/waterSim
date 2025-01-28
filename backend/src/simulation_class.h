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

/*
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

        int* deviceWidth; // width of the simulation stored on the device
        int* deviceHeight;// height of the simulation stored on the device
        int* deviceDepth; // depth of the simulation stored on the device

        queue q;

        /**
         * velocity x,y,z 
         * and ability of that velocity to change in the w component 
         *      (0.0f to 1.0f) 0 meaning can't change (aka a wall) 1 meaning it can change to the fullest extent
         */
        float4* deviceVelocityArr; // device arr 1 and has length equal to arrLen 

        float* deviceDensityArr; // device arr 2 and has length equal to arrLen 
        
        // the divergence of the velocities of a cell are mutiplied by this, it helps to keep the simulation from collapsing
        const float overRelax = 1.0f;
        float* deviceOverRelax; // overrelax value stored on device

        float4* velocityVectors; // private host velocity array

    public:
        const int* const pWidth = &width;
        const int* const pHeight = &height;
        const int* const pDepth = &depth;

        const int* const pAmtOfCells = &arrLen;
        const int* const pVelocityArrLen = &arrLen;

        //x and y and z velocity vector components + w for ability of that velocity to change 
        float4* vectors;
        // densities, one per individual cell
        float* densities;

    //constructor
    Simulation(int width, int height, int depth)
    {
        this->q = queue(handle_async_error);
        std::cout << "running simulation on -> " << q.get_device().get_info<info::device::name>() << std::endl;

        this->height = height;
        this->width = width;
        this->depth = depth;

        this->arrLen = this->width * this->height * this->depth;

        this->velocityVectors = new float4[this->arrLen];
        this->vectors = velocityVectors;

        this->deviceDensityArr = new float[this->arrLen];
        this->densities = this->deviceDensityArr;


        // allocate enough space in the device for the array of vectors
        this->deviceVelocityArr = malloc_device<float4>(this->arrLen * sizeof(float4), q);

        // allocate enough space in the device for the array of densities
        this->deviceDensityArr = malloc_device<float>(this->arrLen * sizeof(float), q);

        // allocate space for the width, height, and depth integers
        this->deviceWidth = malloc_device<int>(sizeof(int), this->q); 
        this->deviceHeight = malloc_device<int>(sizeof(int), this->q);
        this->deviceDepth = malloc_device<int>(sizeof(int), this->q);

        this->deviceOverRelax = malloc_device<float>(sizeof(float), this->q);

        // copy vectors to the device
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceVelocityArr, this->velocityVectors, this->arrLen * sizeof(float4));
        });

        // copy densities to the device
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceDensityArr, this->densities, this->arrLen * sizeof(float));
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

        //wait for all the memcpys to complete
        this->q.wait();
    }

    // deconstructor
    ~Simulation()
    {
        // free the allocated memory in the device
        free(this->deviceVelocityArr, q); 
        free(this->deviceDensityArr, q); 
        free(this->deviceWidth, q);
        free(this->deviceHeight, q);
        free(this->deviceDepth, q);

        delete[] this->velocityVectors; // free the array memory
        delete[] this->densities; // x2
    }

    /**
     * recopy the density, velocity arrays, and the width, height, depth, and overrelax to the device attached to the queue,
     * overwriting the old data already there
     */
    void send()
    {
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceVelocityArr, this->velocityVectors, this->arrLen * sizeof(float4));
        });

        // copy densities to the device
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceDensityArr, this->densities, this->arrLen * sizeof(float));
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
        float* densityArray = this->deviceDensityArr;
                
        int* pWidth = deviceWidth;
        int* pHeight = deviceHeight;
        int* pDepth = deviceDepth;

        float* pOverRelax = deviceOverRelax;

        // do the math
        event modifyVelocityValues = 
        this->q.submit([&](handler& h) 
        {
            h.parallel_for(this->arrLen, [=](id<1> i) 
            {
                velocityArray[i].y() -= 1.0f * dt * velocityArray[i].w();

                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                int x = i % (width - 1);
                int y = (i / (width - 1)) % (height - 1);
                int z = i / ((width - 1) * (height - 1));

                if(y == 0)
                {
                    velocityArray[i].x() = 0.0f;
                    velocityArray[i].y() = 0.0f;
                    velocityArray[i].z() = 0.0f;
                } 
            });
        });


        event projection = 
        this->q.submit([&](handler& h) 
        {
            h.depends_on(modifyVelocityValues);

            /**
             * this is an idea that is being testing 
             * taking the projection method from the 10 minute physics video: https://www.youtube.com/watch?v=iKAVRgIrUOU&t=27s
             * where there are velocities at the edges of 2d squares and the fluid moving in and out of the squares has to be equal
             * and applying it to a 3d grid of cells where the velocity components are at the center.
             * 
             * by taking every in between cube of 8 velocities and making their velocities match so that the sum of their vectors results in a vector of zero length, 
             * meaning no fluid is moving in or out. 
             * 
             * currently does not work, vectors are being infinitly mutiplied/added to and result in massive velocities that do not make sense in the given context
             * TODO: fix it duh
             */
            h.parallel_for((this->width - 1) * (this->height - 1) * (this->depth - 1), [=](id<1> i) 
            { 
                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                float overRelax = *pOverRelax;

                
                int x = i % (width - 1);
                int y = (i / (width - 1)) % (height - 1);
                int z = i / ((width - 1) * (height - 1)); 

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

                // the projection of magnitude of the vector u when vector u is projected on to vector v is:
                // (vector u dotproduct vector v) divided by the magnitude of vector v

                // magnitude of the vectors projected onto a vector going through the middle of the 8 positions, and the nth position
                float out1 = ((velocity1.x() * -1) + (velocity1.y() * -1) + (velocity1.z() * -1)) / sycl::_V1::sqrt(3.0f); 
                float out2 = ((velocity2.x() *  1) + (velocity2.y() * -1) + (velocity2.z() * -1)) / sycl::_V1::sqrt(3.0f);
                float out3 = ((velocity3.x() * -1) + (velocity3.y() *  1) + (velocity3.z() * -1)) / sycl::_V1::sqrt(3.0f);
                float out4 = ((velocity4.x() * -1) + (velocity4.y() * -1) + (velocity4.z() *  1)) / sycl::_V1::sqrt(3.0f);
                float out5 = ((velocity5.x() * -1) + (velocity5.y() *  1) + (velocity5.z() *  1)) / sycl::_V1::sqrt(3.0f);
                float out6 = ((velocity6.x() *  1) + (velocity6.y() * -1) + (velocity6.z() *  1)) / sycl::_V1::sqrt(3.0f);
                float out7 = ((velocity7.x() *  1) + (velocity7.y() *  1) + (velocity7.z() * -1)) / sycl::_V1::sqrt(3.0f);
                float out8 = ((velocity8.x() *  1) + (velocity8.y() *  1) + (velocity8.z() *  1)) / sycl::_V1::sqrt(3.0f);

                // // not equal to but proportional to the actual amount of outflow
                float divergence = out1 + out2 + out3 + out4 + out5 + out6 + out7 + out8;
                divergence = divergence * overRelax;

                float velocity1change = 1.0f/8.0f * 1.0f/7.0f * (                velocity2.w() + velocity3.w() + velocity4.w() + velocity5.w() + velocity6.w() + velocity7.w() + velocity8.w());
                float velocity2change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() +                 velocity3.w() + velocity4.w() + velocity5.w() + velocity6.w() + velocity7.w() + velocity8.w());
                float velocity3change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() +                 velocity4.w() + velocity5.w() + velocity6.w() + velocity7.w() + velocity8.w());
                float velocity4change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() + velocity3.w() +                 velocity5.w() + velocity6.w() + velocity7.w() + velocity8.w());
                float velocity5change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() + velocity3.w() + velocity4.w() +                 velocity6.w() + velocity7.w() + velocity8.w());
                float velocity6change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() + velocity3.w() + velocity4.w() + velocity5.w() +                 velocity7.w() + velocity8.w());
                float velocity7change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() + velocity3.w() + velocity4.w() + velocity5.w() + velocity6.w() +                 velocity8.w());
                float velocity8change = 1.0f/8.0f * 1.0f/7.0f * (velocity1.w() + velocity2.w() + velocity3.w() + velocity4.w() + velocity5.w() + velocity6.w() + velocity7.w()                );

                velocityArray[x     + (y       * height) + (z       * width * height)] = velocity1 + (float4(-divergence, -divergence, -divergence, 0) * velocity1change * velocity1.w());
                velocityArray[x + 1 + (y       * height) + (z       * width * height)] = velocity2 + (float4( divergence, -divergence, -divergence, 0) * velocity2change * velocity2.w());
                velocityArray[x     + ((y + 1) * height) + (z       * width * height)] = velocity3 + (float4(-divergence,  divergence, -divergence, 0) * velocity3change * velocity3.w());
                velocityArray[x     + (y       * height) + ((z + 1) * width * height)] = velocity4 + (float4(-divergence, -divergence,  divergence, 0) * velocity4change * velocity4.w());
                velocityArray[x     + ((y + 1) * height) + ((z + 1) * width * height)] = velocity5 + (float4(-divergence,  divergence,  divergence, 0) * velocity5change * velocity5.w());
                velocityArray[x + 1 + (y       * height) + ((z + 1) * width * height)] = velocity6 + (float4( divergence, -divergence,  divergence, 0) * velocity6change * velocity6.w());
                velocityArray[x + 1 + ((y + 1) * height) + (z       * width * height)] = velocity7 + (float4( divergence,  divergence, -divergence, 0) * velocity7change * velocity7.w());
                velocityArray[x + 1 + ((y + 1) * height) + ((z + 1) * width * height)] = velocity8 + (float4( divergence,  divergence,  divergence, 0) * velocity8change * velocity8.w());
            });
        });
        

        event advection = 
        this->q.submit([&](handler& h) 
        {
            h.depends_on(projection);

            h.parallel_for(this->arrLen, [=](id<1> i) 
            {
                int width = *pWidth;
                int height = *pHeight;
                int depth = *pDepth;

                int x = i % width;
                int y = (i / width) % height;
                int z = i / (width * height);

                float previousX = x - (velocityArray[i].x() * dt); // position to grab values from, over/under flow positions wrap around 
                float previousY = y - (velocityArray[i].y() * dt); 
                float previousZ = z - (velocityArray[i].z() * dt);

                // previousX = previousX - (width - 1)  * sycl::_V1::floor(previousX / (width - 1)); // mod so that they are within the range: (0 to width)
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
        }); 


        // then copy it back to the vectors host array
        this->q.submit([&](handler& h) 
        {
            h.depends_on(advection);

            h.memcpy(this->velocityVectors, deviceVelocityArr, this->arrLen * sizeof(float4));
        });

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


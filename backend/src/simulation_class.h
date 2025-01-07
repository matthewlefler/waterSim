/*
    name: simulation_class.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for a custom simulation class to model fluids
*/ 

#include <vector>

////////////
//  SYCL  //
////////////
#include<sycl/sycl.hpp>
using namespace sycl;



class Simulation
{
    private:
        int width;
        int height;
        int totalLength;

        queue q;

        float4* deviceArray;

    public:
        const int* const pWidth = &width;
        const int* const pHeight = &height;
        const int* const pTotalLength = &totalLength;

        //x and y and z vector components + w for brightness or another purpose (such as density)
        float4* vectors;

    //constructor
    Simulation(int width, int height)
    {
        this->q = queue(cpu_selector_v);

        this->height = height;
        this->width = width;
        this->totalLength = width * height;

        this->vectors = new float4[this->totalLength];

        // allocate enough space in the device for the array of vectors 
        this->deviceArray = malloc_device<float4>(this->totalLength, q);
        
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceArray, this->vectors, totalLength * sizeof(float4));
        }).wait(); //and wait for it to complete
    }

    // deconstructor
    ~Simulation()
    {
        free(this->deviceArray, q);
        delete[] this->vectors;
    }

    void send()
    {
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(this->deviceArray, this->vectors, totalLength * sizeof(float4));
        }).wait(); //and wait for it to complete
    }

    void next_frame(float dt)
    {
        float4* test = this->deviceArray;
        //do the math
        event changeDeviceData = this->q.submit([&](handler& h) {
            h.parallel_for(this->totalLength, [=](id<1> i) 
            { 
                test[i].w() += 0.1f;

                if( test[i].w() > 1.0f )
                {
                    test[i].w() = 0.0f;
                } 
            });
        });

        //then copy it back to the vectors host array
        this->q.submit([&](handler& h) {
            h.depends_on(changeDeviceData);
            h.memcpy(this->vectors, deviceArray, this->totalLength * sizeof(float4));
        });

        this->q.wait();
    }

    void print()
    {
        for (int i = 0; i < this->totalLength; ++i)
        {
            std::cout << " | " << this->vectors[i].x() << " " << this->vectors[i].y() << " " << this->vectors[i].z() << " " << this->vectors[i].w() << " ";

            if(i % this->width + 1 >= this->width)
            {
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
};
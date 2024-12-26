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

class simulation
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
        std::vector<float4> vectors;

    //constructor
    simulation(int width, int height)
    {
        this->q = queue(cpu_selector_v);

        this->height = height;
        this->width = width;
        this->totalLength = width * height;

        this->vectors = std::vector<float4>(this->totalLength);

        // allocate enough space in the device for the array of vectors 
        this->deviceArray = malloc_device<float4>(this->totalLength, q);
        
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(deviceArray, &this->vectors[0], totalLength * sizeof(float4));
        }).wait(); //and wait for it to complete
    }

    // deconstructor
    ~simulation()
    {
        free(deviceArray, q);
    }

    void send()
    {
        // copy vectors to the deviceArray
        this->q.submit([&](handler& h) {
            h.memcpy(deviceArray, &this->vectors[0], totalLength * sizeof(float4));
        }).wait(); //and wait for it to complete
    }

    void next_frame(float dt)
    {
        float4* test = deviceArray;
        //do the math
        event changeDeviceData = this->q.submit([&](handler& h) {
            h.parallel_for(this->totalLength, [=](id<1> i) { test[i] += float4(0.1 * (double)i, 0.2, 0.3, 0); });
        });

        //then copy it back to the vectors
        this->q.submit([&](handler& h) {
            h.depends_on(changeDeviceData);
            h.memcpy(&this->vectors[0], deviceArray, this->totalLength * sizeof(float4));
        });

        this->q.wait();
    }
};
/*
    name: simulation_class.h
    author: matt l
        slack: @skye 

    usecase:
        defines and provides functions for a custom simulation class to model fluids
*/ 

#include <vector>
#include <iostream>
#include <atomic>

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

        bool vector_switch = 0;
        float4* vectors1; //switches to enable reads at any time;
        float4* vectors2;

    public:
        const int* const pWidth = &width;
        const int* const pHeight = &height;
        const int* const pTotalLength = &totalLength;

        //x and y and z vector components + w for brightness or another purpose (such as density)
        std::atomic<float4*> vectors;

    //constructor
    Simulation(int width, int height)
    {
        this->q = queue(cpu_selector_v);

        this->height = height;
        this->width = width;
        this->totalLength = width * height;

        this->vectors1 = new float4[this->totalLength];
        this->vectors2 = new float4[this->totalLength];

        this->vectors = vectors1;

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
        float4* array = this->deviceArray;
        Simulation* pSim = this;
        //do the math
        event changeDeviceData = this->q.submit([&](handler& h) 
        {
            h.parallel_for(this->totalLength, [=](id<1> i) 
            { 
                array[i].w() += 0.1f;

                if(array[i].w() > 1.0f)
                {
                    array[i].w() = 0.0f;
                }

                /*
                int count = 0;
                if( i % pSim->width  == 0)
                {
                    return;
                }
                if( i / pSim->height == 0 || i / pSim->height == pSim->height )  
                {
                    return;
                }

                if( array[i - 1               ].w() > 0.5f ) { count++; }
                if( array[i + 1               ].w() > 0.5f ) { count++; }
                if( array[i + pSim->height    ].w() > 0.5f ) { count++; }
                if( array[i + pSim->height - 1].w() > 0.5f ) { count++; }
                if( array[i + pSim->height + 1].w() > 0.5f ) { count++; }
                if( array[i - pSim->height - 1].w() > 0.5f ) { count++; }
                if( array[i - pSim->height - 1].w() > 0.5f ) { count++; }
                if( array[i - pSim->height - 1].w() > 0.5f ) { count++; }

                if(count > 4 || count < 1)
                {
                    array[i].w() = 0.0f;
                }
                else if(count == 3)
                {
                    array[i].w() = 1.0f;
                }*/
            });
        });


        if(this->vector_switch)
        {
            //then copy it back to the vectors host array
            this->q.submit([&](handler& h) 
            {
                this->vectors = vectors1;
                this->vector_switch = false;

                h.depends_on(changeDeviceData);
                h.memcpy(this->vectors2, deviceArray, this->totalLength * sizeof(float4));
            });
        }
        else
        {
            //then copy it back to the vectors host array
            this->q.submit([&](handler& h) 
            {
                this->vectors = vectors2;
                this->vector_switch = true;

                h.depends_on(changeDeviceData);
                h.memcpy(this->vectors1, deviceArray, this->totalLength * sizeof(float4));
            });
        }

        this->q.wait();
    }

    void print()
    {
        std::cout << "sim print: "; 
        for (int i = 0; i < this->totalLength; ++i)
        {
            float4 vec = (*(&this->vectors))[i];
            std::cout << " | " << vec.x() << " " << vec.y() << " " << vec.z() << " " << vec.w() << " ";

            if(i % this->width + 1 >= this->width)
            {
                // std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
};
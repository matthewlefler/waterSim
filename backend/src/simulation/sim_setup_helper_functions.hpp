#include <sycl/sycl.hpp>

/**
 * a functor object for a sycl kernal that changes the values to match
 * a cyclinder the height of the simulation aligned along the y axis within a volume of free flowing fluid
 */
class CyclinderInBox
{
    public:
    CyclinderInBox(
        sycl::accessor<bool, 3, sycl::access_mode::write> device_accessor_changeable_buffer,
        sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_1,
        sycl::range<3> dims,
        float radius)
    {
        this->r_squared = radius;

        this->dims = dims;
        
        this->device_accessor_changeable_buffer = sycl::accessor(device_accessor_changeable_buffer);
        this->device_accessor_velocity_buffer_1 = sycl::accessor(device_accessor_velocity_buffer_1);
    }

    void operator() (sycl::id<3> i)
    {
        //assign the stating value of the simulation
        device_accessor_velocity_buffer_1[i] = sycl::_V1::float4(0.0f, 0.0f, 0.0f, 1.0f);
        device_accessor_changeable_buffer[i] = true;

        if(i.get(1) == 0 || i.get(1) == dims.get(1) - 1 || i.get(2) == 0 || i.get(2) == dims.get(2) - 1)
        {
            device_accessor_changeable_buffer[i] = false;
        }

        if(i.get(0)*i.get(0) + i.get(2)*i.get(2) < r_squared)
        {
            device_accessor_changeable_buffer[i] = false;
        }
    }

    private:
    float r_squared;

    sycl::range<3> dims;

    sycl::accessor<bool, 3, sycl::access_mode::write> device_accessor_changeable_buffer;
    sycl::accessor<sycl::float4, 3, sycl::access_mode::write> device_accessor_velocity_buffer_1;
};
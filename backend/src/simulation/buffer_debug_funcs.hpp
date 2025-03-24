#include <iostream> // used for debugging via std out
#include <sycl/sycl.hpp> // the main library used for parrellelism 

/**
 * a collection of functions to help debug values in sycl::buffer objects
 */


/**
 * prints out an amount of values of the given buffer to the std output
 * uses a host accessor to do so and should only be used for debugging purposes
 */
template <typename T> 
void read_out_values(sycl::buffer<T, 1> * buffer, int amount) {

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

    auto accessor = buffer->get_host_access();
    for (int i = 0; i < amount; i++)
    {
        std::cout << "n" << i << ": " << (int) accessor[i] << ", ";
    }
    // flush the buffer
    std::cout << std::endl;
}
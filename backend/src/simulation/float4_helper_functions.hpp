#include <sycl/sycl.hpp>

/**
 * this function does the dotproduct of the xyz componets of the float4 and the x, y, z floats
 */
inline float dotproduct(sycl::_V1::float4 vec, float x, float y, float z) 
{
    return vec.x() * x + vec.y() * y + vec.z() * z;
}

/**
 * a specific implentation of (a dot b / b dot b) * b; where dot is the dotproduct
 * see: https://en.wikipedia.org/wiki/Vector_projection
 */
inline sycl::_V1::float4 Projection3d(sycl::_V1::float4 vec, float x, float y, float z)
{
    float dot = dotproduct(vec, x, y, z) / (x*x + y*y + z*z);
    return sycl::_V1::float4(x * dot, y * dot, z * dot, vec.w());
}

/**
 * returns the magnitude of the first three componenets of parameter vec
 */
inline float magnitude(sycl::_V1::float4 vec)
{
    return sycl::_V1::sqrt(vec.x() * vec.x() + vec.y() * vec.y() + vec.z() * vec.z());
}

/**
 * mutiplies the scalar s by the x,y,z componenets of the vec and returns the new float4
 */
inline sycl::_V1::float4 scale3d(sycl::_V1::float4 vec, float s)
{
    return sycl::_V1::float4(vec.x() * s, vec.y() * s, vec.z() * s, vec.w());
}
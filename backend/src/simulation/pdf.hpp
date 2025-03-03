#include <sycl/sycl.hpp>

#define pi 3.14159265358979323846f

inline float maxwell_boltzmann_pdf(float x, float a)
{
    return sycl::sqrt(2.0f / pi) * (x * x / a) * sycl::exp((-1.0f * x * x) / (2.0f * a * a));
}
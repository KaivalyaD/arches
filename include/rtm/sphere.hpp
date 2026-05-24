#pragma once

#include "vec3.hpp"
#include "ray.hpp"

namespace rtm {

struct Sphere {
    rtm::vec3 center;
    float radius;
};

struct SphereList {
    Sphere *spheres;
    uint sphere_count;
};

}

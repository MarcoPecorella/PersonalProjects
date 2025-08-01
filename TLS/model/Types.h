#pragma once
#include "HandledMessageKeys.h"

#include <array>
#include <vector>

constexpr int triangle_vertexes = 3;


struct point2d {
    double x;
    double y;
};

struct point3d {
    double x;
    double y;
    double z;
};

typedef std::array<point2d, triangle_vertexes> triangle;
typedef std::vector<triangle> triangles;
typedef std::vector<eHandledMessageKeys> message_keys;

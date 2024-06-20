#pragma once

#include "tagged.h"

namespace model {

using Dimention = int;
using Coord = Dimention;

struct Point {
    Coord x, y;
};

struct Size {
    Dimention width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimention dx, dy;
};

template <typename T>
struct ParamPair {
    ParamPair() = default;

    ParamPair(const T& x, const T& y) :
        x_(x),
        y_(y) {}

    T x_;
    T y_;
};

using ParamPairInt = ParamPair<int>;
using ParamPairDouble = ParamPair<double>;

}
#pragma once
#include <string>
#include <utility>

#include "Types.h"

class Track {
public:
    std::string _name;
    point2d _position{};
    int _altitude{};

    Track() = default;

    Track(std::string name, const point2d &position, const int &altitude) : _name(std::move(name)), _position(position),
        _altitude(altitude) {
    };

    ~Track() = default;
};

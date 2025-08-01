#include "model/Radar.h"

int main() {
    constexpr point3d origin {0, 0, 0};
    constexpr double distance = 10;
    message_keys keys;
    keys.push_back(eHandledMessageKeys::CAT_10);

    Radar r(origin, distance, keys, 32, 4);
    Track t, y, u;
    t._altitude = 0;
    t._name = "Radar";
    t._position = {0, 3};

    y._altitude = 0;
    y._name = "Radar";
    y._position = {5, -5};

    u._altitude = 0;
    u._name = "Radar";
    u._position = {-2, 3};

    r.addTrack(t);
    r.addTrack(y);
    r.addTrack(u);

    r.run();
    return 0;
}
#include "Radar.h"

#include <cmath>
#include <iostream>
#include <queue>
#include <ranges>
#include <utility>


// @test purpose:
// ##########################################################
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
// ##########################################################

#include <thread>

#include "Math.h"

Radar::Radar(const point3d &o, const double &d, message_keys k, const int &s, const float &s_t) : _origin(o),
    _distance(d),
    _radar_section(s),
    _scan_time(s_t),
    _message_types(std::move(k)) {
    _single_section_scan_time = static_cast<long>(_scan_time / static_cast<float>(_radar_section) * 1000000);
    // conversion seconds to micro
    std::cout << _single_section_scan_time << std::endl;
    buildTrianglesAroundTheOrigin();

    // @test purpose:
    // ##########################################################
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::perror("socket");
        abort();
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sockfd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::perror("connect");
        close(sockfd);
        abort();
    }
}

Radar::~Radar() = default;

void Radar::run() {
    int td = 0;
    int d = 0;
    while (true) {
        auto tmpTracks = this->_tracks;
        td = 0;
        for (auto & v : std::ranges::reverse_view(_zones)) {
            tmpTracks = checkTracksInVolume(tmpTracks, v, d);
            td += d;
        }
        std::cout << "time delta: " << td << std::endl;
    }
}

std::vector<Track>
Radar::checkTracksInVolume(const std::vector<Track> &tracks, const triangle &v, int &timeDelta) const {
    /**
     * This function check in each volume if a track is the triangle,
     * if so, ordinate them in vertical order, then encode the requested
     * messages and send them, eliminate it from the vector and sleep for
     * the restant time for the current sector.
     * @example: function done in 100 milli, the function shall run for 300 milli,
     * there will be a 200 milli sleep.
     */

    const auto start = getCurrentTime<microseconds>();

    std::vector<Track> tmpTracks;
    std::vector<Track> retTracks;

    sendCurrentTriangle(sockfd, v);

    for (auto &t: tracks) {
        if (!checkSingleTrackInVolume(t, v)) {
            retTracks.push_back(t);
        } else {
            auto dist = std::sqrt(
                                square(t._position.x - _origin.x) +
                                  square(t._position.y - _origin.y)
                                  );

            if (t._altitude < std::sqrt(square(_radar_section) - square(dist))) {
                tmpTracks.push_back(t);
            }
        }
    }

    if (tmpTracks.size() > 1) {
        auto cmp = [&](const Track *a, const Track *b) {
            // Primary key: distance from the line
            const double da = distanceFromPoint(a->_position, v.at(1), v.at(2));
            if (const double db = distanceFromPoint(b->_position, v.at(1), v.at(2)); da != db)
                return da > db; // larger → lower priority

            // Secondary key: distance from origin
            const double ca = distanceFromOrigin(*a, _origin);
            const double cb = distanceFromOrigin(*b, _origin);
            return ca > cb; // larger → lower priority
        };

        // Then build your PQ over Track*:
        std::priority_queue<Track *, std::vector<Track *>, decltype(cmp)> pq(cmp);

        // And push pointers:
        for (auto &trk: tmpTracks)
            pq.push(&trk);

        while (!pq.empty()) {
            const auto &trk = pq.top();
            sendTrackPosition(sockfd, *trk);
        }
    } else if (!tmpTracks.empty()) {
        sendTrackPosition(sockfd, tmpTracks[0]);
    }

    if (const auto duration = getCurrentTime<microseconds>() - start; duration.count() < _single_section_scan_time) {
        std::this_thread::sleep_for(microseconds(_single_section_scan_time - duration.count()));
    }

    timeDelta = (getCurrentTime<microseconds>() - start).count();
    return retTracks;
}

bool Radar::checkSingleTrackInVolume(const Track &t, const triangle &v) {
    bool inside = false;
    for (size_t i = 0, j = v.size() - 1; i < v.size(); j = i++) {
        const auto& pi = v[i];
        const auto& pj = v[j];

        // Does the horizontal ray through p cross edge pj->pi?
        const bool intersect = ((pi.y > t._position.y) != (pj.y > t._position.y))
                      && ( t._position.x < (pj.x - pi.x) * (t._position.y - pi.y)
                                 / (pj.y - pi.y)
                                + pi.x);
        if (intersect)
            inside = !inside;
    }
    return inside;
}

template<class T>
T Radar::getCurrentTime() {
    return duration_cast<T>(system_clock::now().time_since_epoch());
}

void Radar::getLine(const point2d &p1, const point2d &p2, double &alpha, double &beta, double &gamma) {
    alpha = p1.y - p2.y;
    beta = p2.x - p1.x;
    gamma = p1.x * p1.y - p2.x * p1.y;
}

double Radar::distanceFromOrigin(const Track &t, const point3d &p) {
    return std::hypot(t._position.x - p.x, t._position.y - p.y);;
}

double Radar::distanceFromPoint(const point2d &origin, const point2d &line_vertex_1, const point2d &line_vertex_2) {
    double a, b, c;
    getLine(line_vertex_1, line_vertex_2, a, b, c);
    return std::abs(a * origin.x + b * origin.y + c) / sqrt(a * a + b * b);
}


void Radar::buildTrianglesAroundTheOrigin() {
    const auto start = getCurrentTime<microseconds>();

    const double angle_section = 360.0 / _radar_section;
    constexpr double conv_to_rad = M_PI / 180;

    for (int i = 0; i < _radar_section; ++i) {
        const double alpha = (90.0 + i * angle_section) * conv_to_rad;
        const double beta = (90.0 + (i + 1) * angle_section) * conv_to_rad;

        _zones.push_back(triangle({
            {_origin.x, _origin.y},
            {std::cos(alpha) * _distance, std::sin(alpha) * _distance},
            {std::cos(beta) * _distance, std::sin(beta) * _distance},
        }));
    }

    const auto total = getCurrentTime<microseconds>() - start;

    std::cout << "Total time required for building the triangles: " << std::to_string(total.count()) << std::endl;

    for (auto t: _zones) {
        for (auto v: t) {
            std::cout << v.x << ", " << v.y << " ";
        }
        std::cout << std::endl;
    }
}

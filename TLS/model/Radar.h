#pragma once
#include <chrono>
#include <iostream>

#include "Types.h"
#include "Track.h"

// @test purpose:
// ##########################################################
#include <sys/socket.h>   // for send()
// ##########################################################

using namespace std::chrono;

class Radar {
public:
    Radar() = delete;
    Radar(const point3d &, const double &, message_keys , const int &, const float &);
    ~Radar();

    void run();

    void addTrack(const Track & t) {
        this->_tracks.push_back(t);
    };

    template<class T>
    static T getCurrentTime();

    static bool checkSingleTrackInVolume(const Track &, const triangle &);

    static double distanceFromOrigin(const Track &, const point3d &);
    static void getLine(const point2d &, const point2d &, double &, double &, double &);
    static double distanceFromPoint(const point2d &, const point2d &, const point2d &);

private:
    std::vector<Track> checkTracksInVolume(const std::vector<Track> &, const triangle &, int &) const;
    void buildTrianglesAroundTheOrigin();

    point3d _origin; // lat, lon, altitude
    double _distance; // distance in m
    int _radar_section;

    float _scan_time; // time in s which indicate the time needed for scan time
    long _single_section_scan_time;

    triangles _zones; // 360 triangle around origin
    message_keys _message_types;
    // a set of message types with define which messages the radar sends (like CAT10 or CAT21)

    std::vector<Track> _tracks;

    // @test purpose:
    // ##########################################################
    int sockfd;

    static bool sendTrackPosition(int sockfd, const Track &trk) {
        // 1) Build a JSON line
        std::string msg = "{"
            "\"x\":" + std::to_string(trk._position.x) + ","
            "\"y\":" + std::to_string(trk._position.y) +
        "}\n";

        // 2) Send the bytes
        size_t to_send = msg.size();
        const char *buf = msg.data();
        while (to_send > 0) {
            const ssize_t sent = ::send(sockfd, buf, to_send, 0);
            if (sent < 0) {
                if (errno == EINTR) continue;  // try again
                return false;
            }
            buf      += sent;
            to_send  -= sent;
        }
        return true;
    }

    static bool sendCurrentTriangle(int sockfd, const triangle &t) {
        // 1) Build a JSON line
        std::string msg = "{"
            "\"v1\": [0,0],"
            "\"v2\": [" + std::to_string(t[1].x) + "," + std::to_string(t[1].y) + "],"
            "\"v3\": [" + std::to_string(t[2].x) + "," + std::to_string(t[2].y) + "]"
        "}\n";

        // 2) Send the bytes
        size_t to_send = msg.size();
        const char *buf = msg.data();
        while (to_send > 0) {
            const ssize_t sent = ::send(sockfd, buf, to_send, 0);
            if (sent < 0) {
                if (errno == EINTR) continue;  // try again
                return false;
            }
            buf      += sent;
            to_send  -= sent;
        }
        return true;
    }
};

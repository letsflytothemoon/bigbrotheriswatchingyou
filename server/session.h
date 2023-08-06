#pragma once
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace chrono = std::chrono;
using time_point = chrono::time_point<chrono::system_clock>;

struct Session
{
    time_point started;
    time_point stopped;
    std::string address;
    bool finished = false;

    bool operator < (const Session& otherSession) const
    {
        return started < otherSession.started;
    }

    
    static std::string FromatTimePoint(const time_point& timePoint)
    {
        std::stringstream resultStream;
        std::time_t time = time_point::clock::to_time_t(timePoint);
        tm localTime;
        localtime_s(&localTime, &time);
        resultStream << localTime.tm_year + 1900
            << "-" << localTime.tm_mon + 1
            << "-" << localTime.tm_mday
            << " " << localTime.tm_hour
            << ":" << localTime.tm_min
            << ":" << localTime.tm_sec;
        return resultStream.str();
    }
};

template <class Stream>
Stream& operator <<(Stream& stream, const Session& session)
{
    stream << "{ \"started\" : \"" << Session::FromatTimePoint(session.started) << "\", ";
    if (session.finished)
        stream << "\"stopped\" : \"" << Session::FromatTimePoint(session.stopped) << "\", ";
    stream << "\"address\" : \"" << session.address << "\" }";
    return stream;
}
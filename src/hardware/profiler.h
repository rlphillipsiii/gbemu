#ifndef _PROFILER_H
#define _PROFILER_H

#include <chrono>
#include <string>

#include "logging.h"

class TimeProfiler {
public:
    TimeProfiler(const std::string & message)
        : m_message(message)
    {
#ifdef PROFILING        
        m_start = std::chrono::high_resolution_clock::now();
#endif        
    }

    ~TimeProfiler()
    {
#ifdef PROFILING
        auto finish = std::chrono::high_resolution_clock::now();

        int ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - m_start).count();
        TRACE("%s: %dms\n", m_message.c_str(), ms);
#endif
    }

private:
    std::string m_message;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};
#endif

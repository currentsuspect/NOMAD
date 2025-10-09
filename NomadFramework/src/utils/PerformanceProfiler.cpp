/**
 * @file PerformanceProfiler.cpp
 * @brief Performance profiling utilities implementation
 */

#include "utils/PerformanceProfiler.h"

namespace nomad::utils
{
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> PerformanceProfiler::startTimes;
    std::unordered_map<std::string, double> PerformanceProfiler::timings;
    
    void PerformanceProfiler::startTimer(const std::string& name)
    {
        startTimes[name] = std::chrono::high_resolution_clock::now();
    }
    
    double PerformanceProfiler::stopTimer(const std::string& name)
    {
        auto it = startTimes.find(name);
        if (it != startTimes.end())
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(endTime - it->second);
            double elapsed = duration.count();
            timings[name] = elapsed;
            startTimes.erase(it);
            return elapsed;
        }
        return 0.0;
    }
    
    double PerformanceProfiler::getTiming(const std::string& name)
    {
        auto it = timings.find(name);
        return it != timings.end() ? it->second : 0.0;
    }
    
    void PerformanceProfiler::clearTimings()
    {
        startTimes.clear();
        timings.clear();
    }
}
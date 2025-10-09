/**
 * @file PerformanceProfiler.h
 * @brief Performance profiling utilities
 * @author Nomad Framework Team
 */

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace nomad::utils
{
    /**
     * @class PerformanceProfiler
     * @brief Simple performance profiler for measuring execution time
     */
    class PerformanceProfiler
    {
    public:
        /**
         * @brief Start timing a section
         * @param name Section name
         */
        static void startTimer(const std::string& name);
        
        /**
         * @brief Stop timing a section
         * @param name Section name
         * @return Elapsed time in milliseconds
         */
        static double stopTimer(const std::string& name);
        
        /**
         * @brief Get timing for a section
         * @param name Section name
         * @return Elapsed time in milliseconds
         */
        static double getTiming(const std::string& name);
        
        /**
         * @brief Clear all timings
         */
        static void clearTimings();
        
    private:
        static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> startTimes;
        static std::unordered_map<std::string, double> timings;
    };
}
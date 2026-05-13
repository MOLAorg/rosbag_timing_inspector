/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct TopicTimingData
{
    std::string topic_name;
    std::string message_type;

    // Timestamps in nanoseconds (from bag clock)
    std::vector<int64_t> timestamps_ns;

    // Derived: inter-message intervals in seconds
    std::vector<double> intervals_s;

    double mean_interval_s  = 0.0;
    double std_interval_s   = 0.0;
    double min_interval_s   = 0.0;
    double max_interval_s   = 0.0;
    double mean_frequency_hz = 0.0;
};

struct BagTimingData
{
    std::string bag_path;
    int64_t     start_time_ns = 0;
    int64_t     end_time_ns   = 0;

    std::vector<TopicTimingData> topics;

    bool empty() const { return topics.empty(); }

    double duration_s() const
    {
        return static_cast<double>(end_time_ns - start_time_ns) * 1e-9;
    }
};

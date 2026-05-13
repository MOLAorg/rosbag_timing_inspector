/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#include "BagReader.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>

#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>

BagTimingData read_bag_timing(const std::string& bag_path)
{
    BagTimingData result;
    result.bag_path = bag_path;

    rosbag2_storage::StorageOptions storage_opts;
    storage_opts.uri = bag_path;
    // Leave storage_id empty so rosbag2 auto-detects mcap vs db3.

    rosbag2_cpp::ConverterOptions converter_opts;
    converter_opts.input_serialization_format  = "cdr";
    converter_opts.output_serialization_format = "cdr";

    rosbag2_cpp::Reader reader;
    reader.open(storage_opts, converter_opts);

    // Collect metadata for known topics
    const auto& metadata = reader.get_metadata();
    std::unordered_map<std::string, size_t> topic_index;

    for (const auto& topic_info : metadata.topics_with_message_count)
    {
        TopicTimingData ttd;
        ttd.topic_name   = topic_info.topic_metadata.name;
        ttd.message_type = topic_info.topic_metadata.type;
        ttd.timestamps_ns.reserve(topic_info.message_count);
        topic_index[ttd.topic_name] = result.topics.size();
        result.topics.push_back(std::move(ttd));
    }

    // Iterate messages -- only read header, skip payload
    while (reader.has_next())
    {
        auto msg = reader.read_next();
        auto it  = topic_index.find(msg->topic_name);
        if (it != topic_index.end())
        {
            result.topics[it->second].timestamps_ns.push_back(
                static_cast<int64_t>(msg->send_timestamp));
        }
    }

    // Compute global time range and per-topic stats
    result.start_time_ns = INT64_MAX;
    result.end_time_ns   = INT64_MIN;

    for (auto& ttd : result.topics)
    {
        if (ttd.timestamps_ns.empty())
        {
            continue;
        }

        std::sort(ttd.timestamps_ns.begin(), ttd.timestamps_ns.end());

        const int64_t t0 = ttd.timestamps_ns.front();
        const int64_t t1 = ttd.timestamps_ns.back();
        result.start_time_ns = std::min(result.start_time_ns, t0);
        result.end_time_ns   = std::max(result.end_time_ns, t1);

        const size_t n = ttd.timestamps_ns.size();
        if (n < 2)
        {
            continue;
        }

        ttd.intervals_s.resize(n - 1);
        for (size_t i = 0; i < n - 1; i++)
        {
            ttd.intervals_s[i] =
                static_cast<double>(ttd.timestamps_ns[i + 1] - ttd.timestamps_ns[i]) * 1e-9;
        }

        const double sum =
            std::accumulate(ttd.intervals_s.begin(), ttd.intervals_s.end(), 0.0);
        ttd.mean_interval_s = sum / static_cast<double>(ttd.intervals_s.size());

        double sq_sum = 0.0;
        for (const double v : ttd.intervals_s)
        {
            const double d = v - ttd.mean_interval_s;
            sq_sum += d * d;
        }
        ttd.std_interval_s = std::sqrt(sq_sum / static_cast<double>(ttd.intervals_s.size()));
        ttd.min_interval_s =
            *std::min_element(ttd.intervals_s.begin(), ttd.intervals_s.end());
        ttd.max_interval_s =
            *std::max_element(ttd.intervals_s.begin(), ttd.intervals_s.end());
        ttd.mean_frequency_hz =
            (ttd.mean_interval_s > 0.0) ? (1.0 / ttd.mean_interval_s) : 0.0;
    }

    if (result.start_time_ns == INT64_MAX)
    {
        result.start_time_ns = 0;
        result.end_time_ns   = 0;
    }

    return result;
}

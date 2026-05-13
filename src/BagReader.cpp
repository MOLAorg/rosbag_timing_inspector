/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#include "BagReader.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <unordered_map>

// rosbag2_storage < 0.17 (Humble) used `time_stamp`.
// 0.17+ (Iron, Jazzy, ...) renamed it to `recv_timestamp`.
#if (ROSBAG2_STORAGE_VERSION_MAJOR == 0 && ROSBAG2_STORAGE_VERSION_MINOR < 17)
#define ROSBAG2_MSG_TIMESTAMP(msg) static_cast<int64_t>((msg)->time_stamp)
#else
#define ROSBAG2_MSG_TIMESTAMP(msg) static_cast<int64_t>((msg)->recv_timestamp)
#endif

// ---------------------------------------------------------------------------
// Internal implementation shared by both sync and async paths
// ---------------------------------------------------------------------------
static void do_read(
    const std::string& bag_path, BagTimingData& result,
    LoadProgress* progress)  // may be nullptr for sync path
{
  result.bag_path = bag_path;

  rosbag2_storage::StorageOptions storage_opts;
  storage_opts.uri = bag_path;

  rosbag2_cpp::ConverterOptions converter_opts;
  converter_opts.input_serialization_format  = "cdr";
  converter_opts.output_serialization_format = "cdr";

  rosbag2_cpp::Reader reader;
  reader.open(storage_opts, converter_opts);

  const auto&                             metadata = reader.get_metadata();
  std::unordered_map<std::string, size_t> topic_index;

  uint64_t total_messages = 0;
  for (const auto& topic_info : metadata.topics_with_message_count)
  {
    TopicTimingData ttd;
    ttd.topic_name   = topic_info.topic_metadata.name;
    ttd.message_type = topic_info.topic_metadata.type;
    ttd.timestamps_ns.reserve(topic_info.message_count);
    topic_index[ttd.topic_name] = result.topics.size();
    result.topics.push_back(std::move(ttd));
    total_messages += topic_info.message_count;
  }

  if (progress != nullptr)
  {
    progress->messages_total.store(total_messages);
  }

  uint64_t msgs_read = 0;
  while (reader.has_next())
  {
    auto msg = reader.read_next();
    auto it  = topic_index.find(msg->topic_name);
    if (it != topic_index.end())
    {
      result.topics[it->second].timestamps_ns.push_back(ROSBAG2_MSG_TIMESTAMP(msg));
    }
    ++msgs_read;
    if (progress != nullptr)
    {
      progress->messages_read.store(msgs_read);
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

    const int64_t t0     = ttd.timestamps_ns.front();
    const int64_t t1     = ttd.timestamps_ns.back();
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

    const double sum    = std::accumulate(ttd.intervals_s.begin(), ttd.intervals_s.end(), 0.0);
    ttd.mean_interval_s = sum / static_cast<double>(ttd.intervals_s.size());

    double sq_sum = 0.0;
    for (const double v : ttd.intervals_s)
    {
      const double d = v - ttd.mean_interval_s;
      sq_sum += d * d;
    }
    ttd.std_interval_s    = std::sqrt(sq_sum / static_cast<double>(ttd.intervals_s.size()));
    ttd.min_interval_s    = *std::min_element(ttd.intervals_s.begin(), ttd.intervals_s.end());
    ttd.max_interval_s    = *std::max_element(ttd.intervals_s.begin(), ttd.intervals_s.end());
    ttd.mean_frequency_hz = (ttd.mean_interval_s > 0.0) ? (1.0 / ttd.mean_interval_s) : 0.0;
  }

  if (result.start_time_ns == INT64_MAX)
  {
    result.start_time_ns = 0;
    result.end_time_ns   = 0;
  }
}

// ---------------------------------------------------------------------------

BagTimingData read_bag_timing(const std::string& bag_path)
{
  BagTimingData result;
  do_read(bag_path, result, nullptr);
  return result;
}

void read_bag_timing_async(
    const std::string& bag_path, BagTimingData* result, LoadProgress* progress)
{
  try
  {
    do_read(bag_path, *result, progress);
  }
  catch (const std::exception& e)
  {
    progress->error_message = e.what();
    progress->failed.store(true);
  }
  progress->done.store(true);
}

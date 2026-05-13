/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <atomic>
#include <string>

#include "TimingData.hpp"

// Progress info shared between the reader thread and the GUI thread.
struct LoadProgress
{
  std::atomic<uint64_t> messages_read{0};
  std::atomic<uint64_t> messages_total{0};  // 0 = unknown
  std::atomic<bool>     done{false};
  std::atomic<bool>     failed{false};
  std::string           error_message;  // written before done=true when failed=true
};

// Synchronous read (blocks until complete).
BagTimingData read_bag_timing(const std::string& bag_path);

// Asynchronous read: fills *result when done, updates *progress each message.
// Caller must keep both pointers valid until progress->done is true.
void read_bag_timing_async(
    const std::string& bag_path, BagTimingData* result, LoadProgress* progress);

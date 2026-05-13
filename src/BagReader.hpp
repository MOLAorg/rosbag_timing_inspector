/* rosbag_timing_inspector
 *
 * Copyright (C) 2026 Jose Luis Blanco Claraco
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "TimingData.hpp"
#include <string>

// Reads a rosbag2 file (mcap or db3) and populates BagTimingData.
// Only timestamps and topic names are extracted -- message payloads are not
// deserialized.
BagTimingData read_bag_timing(const std::string& bag_path);

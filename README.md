# rosbag_timing_inspector

A GUI tool to visualize and analyze message timing from ROS 2 bags (`.mcap` or `.db3`).

Built with [Dear ImGui](https://github.com/ocornut/imgui) + [ImPlot](https://github.com/epezent/implot).
No message deserialization is performed -- only timestamps and topic names are read,
so it opens even bags with custom or unavailable message types.

<img width="1009" height="557" alt="rosbag_timing_inspector" src="https://github.com/user-attachments/assets/e527bbd3-4e0e-424c-9500-ed6708bb2885" />


## Features

- **Timeline tab**: zoomable 2D scatter plot -- X = time, Y = topic lane.
  Each dot is one message. Pan/zoom with mouse. Toggle per-topic visibility and color.
  **Measure mode**: click twice to place two vertical markers and read the delta-T.
- **Histograms / Stats tab**: per-topic inter-message interval histogram and a
  statistics table (count, mean, std, min, max in ms).

## Dependencies

System packages (Ubuntu / ROS 2 Jazzy):

```bash
sudo apt install libglfw3-dev
```

ROS 2 packages resolved automatically by `rosdep`:

- `rosbag2_cpp`
- `rosbag2_storage`

Storage backend plugins (installed with a typical ROS 2 desktop install):

- `rosbag2_storage_mcap` — for `.mcap` bags
- `rosbag2_storage_sqlite3` — for `.db3` bags

Third-party libraries (`imgui`, `implot`, `tinyfiledialogs`) are vendored as git
submodules under `third_party/` -- no manual installation needed, but make sure
submodules are checked out (see Build section below).

## Build

```bash
cd ~/ros2_ws/src/rosbag_timing_inspector

# Fetch the vendored third-party submodules (imgui, implot, tinyfiledialogs)
git submodule update --init --recursive

cd ~/ros2_ws

# Install ROS 2 dependencies
rosdep install --from-paths src/rosbag_timing_inspector --ignore-src -r -y

# Build (release build recommended for large bags)
colcon build --packages-select rosbag_timing_inspector \
             --cmake-args -DCMAKE_BUILD_TYPE=Release
```

## Launch

```bash
source ~/ros2_ws/install/setup.bash

# Pass a bag path directly:
rosbag_timing_inspector /path/to/recording.mcap
rosbag_timing_inspector /path/to/recording.db3

# Or open without arguments and use File > Open bag...:
rosbag_timing_inspector
```

## Usage tips

| Action | How |
|---|---|
| Zoom timeline | Scroll wheel over plot, or click-drag on an axis |
| Pan timeline | Middle-click drag, or right-click drag |
| Reset view | Double-click on plot |
| Toggle topic | Checkbox in the left panel |
| Change topic color | Color swatch in the left panel |
| Measure delta-T | Enable **Measure mode**, click for T1, click again for T2; drag either line to adjust |
| Reset measurement | Click a third time to restart |

## ROS build farm status

| Distro | Develop branch | Releases | Stable release |
| ---    | ---            | ---      |  ---      |
| ROS 2 Humble @ u22.04 | [![Build Status](https://build.ros2.org/job/Hdev__rosbag_timing_inspector__ubuntu_jammy_amd64/badge/icon)](https://build.ros2.org/job/Hdev__rosbag_timing_inspector__ubuntu_jammy_amd64/) | amd64 [![Build Status](https://build.ros2.org/job/Hbin_uJ64__rosbag_timing_inspector__ubuntu_jammy_amd64__binary/badge/icon)](https://build.ros2.org/job/Hbin_uJ64__rosbag_timing_inspector__ubuntu_jammy_amd64__binary/) <br> arm64 [![Build Status](https://build.ros2.org/job/Hbin_ujv8_uJv8__rosbag_timing_inspector__ubuntu_jammy_arm64__binary/badge/icon)](https://build.ros2.org/job/Hbin_ujv8_uJv8__rosbag_timing_inspector__ubuntu_jammy_arm64__binary/) | [![Version](https://img.shields.io/ros/v/humble/rosbag_timing_inspector)](https://index.ros.org/?search_packages=true&pkgs=rosbag_timing_inspector) |
| ROS 2 Jazzy @ u24.04 | [![Build Status](https://build.ros2.org/job/Jdev__rosbag_timing_inspector__ubuntu_noble_amd64/badge/icon)](https://build.ros2.org/job/Jdev__rosbag_timing_inspector__ubuntu_noble_amd64/) | amd64 [![Build Status](https://build.ros2.org/job/Jbin_uN64__rosbag_timing_inspector__ubuntu_noble_amd64__binary/badge/icon)](https://build.ros2.org/job/Jbin_uN64__rosbag_timing_inspector__ubuntu_noble_amd64__binary/) <br> arm64 [![Build Status](https://build.ros2.org/job/Jbin_unv8_uNv8__rosbag_timing_inspector__ubuntu_noble_arm64__binary/badge/icon)](https://build.ros2.org/job/Jbin_unv8_uNv8__rosbag_timing_inspector__ubuntu_noble_arm64__binary/) | [![Version](https://img.shields.io/ros/v/jazzy/rosbag_timing_inspector)](https://index.ros.org/?search_packages=true&pkgs=rosbag_timing_inspector) |
| ROS 2 Kilted @ u24.04 | [![Build Status](https://build.ros2.org/job/Kdev__rosbag_timing_inspector__ubuntu_noble_amd64/badge/icon)](https://build.ros2.org/job/Kdev__rosbag_timing_inspector__ubuntu_noble_amd64/) | amd64 [![Build Status](https://build.ros2.org/job/Kbin_uN64__rosbag_timing_inspector__ubuntu_noble_amd64__binary/badge/icon)](https://build.ros2.org/job/Kbin_uN64__rosbag_timing_inspector__ubuntu_noble_amd64__binary/) <br> arm64 [![Build Status](https://build.ros2.org/job/Kbin_unv8_uNv8__rosbag_timing_inspector__ubuntu_noble_arm64__binary/badge/icon)](https://build.ros2.org/job/Kbin_unv8_uNv8__rosbag_timing_inspector__ubuntu_noble_arm64__binary/) | [![Version](https://img.shields.io/ros/v/kilted/rosbag_timing_inspector)](https://index.ros.org/?search_packages=true&pkgs=rosbag_timing_inspector) |
| ROS 2 Lyrical @ u26.04 | [![Build Status](https://build.ros2.org/job/Ldev__rosbag_timing_inspector__ubuntu_resolute_amd64/badge/icon)](https://build.ros2.org/job/Ldev__rosbag_timing_inspector__ubuntu_resolute_amd64/) | amd64 [![Build Status](https://build.ros2.org/job/Lbin_uR64__rosbag_timing_inspector__ubuntu_resolute_amd64__binary/badge/icon)](https://build.ros2.org/job/Lbin_uR64__rosbag_timing_inspector__ubuntu_resolute_amd64__binary/) <br> arm64 [![Build Status](https://build.ros2.org/job/Lbin_urv8_uRv8__rosbag_timing_inspector__ubuntu_resolute_arm64__binary/badge/icon)](https://build.ros2.org/job/Lbin_urv8_uRv8__rosbag_timing_inspector__ubuntu_resolute_arm64__binary/) | [![Version](https://img.shields.io/ros/v/lyrical/rosbag_timing_inspector)](https://index.ros.org/?search_packages=true&pkgs=rosbag_timing_inspector) |
| ROS 2 Rolling (u26.04) | [![Build Status](https://build.ros2.org/job/Rdev__rosbag_timing_inspector__ubuntu_resolute_amd64/badge/icon)](https://build.ros2.org/job/Rdev__rosbag_timing_inspector__ubuntu_resolute_amd64/) | amd64 [![Build Status](https://build.ros2.org/job/Rbin_uR64__rosbag_timing_inspector__ubuntu_resolute_amd64__binary/badge/icon)](https://build.ros2.org/job/Rbin_uR64__rosbag_timing_inspector__ubuntu_resolute_amd64__binary/) <br> arm64 [![Build Status](https://build.ros2.org/job/Rbin_unv8_uRv8__rosbag_timing_inspector__ubuntu_resolute_arm64__binary/badge/icon)](https://build.ros2.org/job/Rbin_unv8_uRv8__rosbag_timing_inspector__ubuntu_resolute_arm64__binary/) | [![Version](https://img.shields.io/ros/v/rolling/rosbag_timing_inspector)](https://index.ros.org/?search_packages=true&pkgs=rosbag_timing_inspector) |

## License

MIT -- Copyright (C) 2026 Jose Luis Blanco Claraco

# rosbag_timing_inspector

A GUI tool to visualize and analyze message timing from ROS 2 bags (`.mcap` or `.db3`).

Built with [Dear ImGui](https://github.com/ocornut/imgui) + [ImPlot](https://github.com/epezent/implot).
No message deserialization is performed -- only timestamps and topic names are read,
so it opens even bags with custom or unavailable message types.

<img width="1398" height="855" alt="rosbag_timing_inspector" src="https://github.com/user-attachments/assets/e4bd15a9-e681-4194-ae0c-cca8095bcddb" />


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
- `rosbag2_storage_mcap`
- `rosbag2_storage_sqlite3`

Third-party libraries (`imgui`, `implot`, `tinyfiledialogs`) are fetched automatically
by CMake via `FetchContent` at build time -- no manual installation needed.

## Build

```bash
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

## License

MIT -- Copyright (C) 2026 Jose Luis Blanco Claraco

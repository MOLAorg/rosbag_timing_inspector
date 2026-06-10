^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rosbag_timing_inspector
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.0.0 (2026-06-10)
------------------
* fix: add opengl and libglfw3-dev as build dependencies
  Required by the ImGui/ImPlot renderer but missing from package.xml,
  causing ROS build farm failures (CMake could not find OpenGL/GLFW).
* fix: remove storage plugins from package.xml dependencies
  rosbag2_storage_mcap and rosbag2_storage_sqlite3 are auto-discovered
  runtime plugins and are not valid rosdep keys, causing build farm
  failures. They ship with a standard ROS 2 desktop install.
* fix: declare storage plugins as exec_depend, not depend
  rosbag2_storage_mcap and rosbag2_storage_sqlite3 are runtime-loaded
  plugins; the code only includes headers from rosbag2_cpp/rosbag2_storage.
  Using <depend> caused build farm rosdep resolution failures.
* docs: add Humble row to ROS build farm status table
* docs: add ROS build farm status badge table (Jazzy, Kilted, Lyrical, Rolling)
* Replace image in README with updated version
  Updated image in README with a new source and dimensions.
* Update README with image and features details
  Added an image to the README and expanded the features section.
* add more visual features
* install exe in bin
* Initial commit
* Contributors: Jose Luis Blanco-Claraco

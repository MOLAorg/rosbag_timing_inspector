cmake_minimum_required(VERSION 3.16)
project(rosbag_timing_inspector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage REQUIRED)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Dear ImGui, ImPlot and tinyfiledialogs are vendored as git submodules under
# third_party/ -- the ROS build farm release jobs build from source tarballs
# without live network access, so these cannot be fetched at build time.
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMPLOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot)
set(TINYFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyfiledialogs)

if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
  message(FATAL_ERROR
    "third_party/imgui is empty -- run "
    "'git submodule update --init --recursive' in the package source tree.")
endif()

# ---- Dear ImGui ----
add_library(imgui_lib STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# ---- ImPlot ----
add_library(implot_lib STATIC
  ${IMPLOT_DIR}/implot.cpp
  ${IMPLOT_DIR}/implot_items.cpp
)
target_include_directories(implot_lib PUBLIC ${IMPLOT_DIR})
target_link_libraries(implot_lib PUBLIC imgui_lib)

# ---- tinyfiledialogs ----
add_library(tinyfd_lib STATIC
  ${TINYFD_DIR}/tinyfiledialogs.c
)
target_include_directories(tinyfd_lib PUBLIC ${TINYFD_DIR})

# ---- Main executable ----
add_executable(rosbag_timing_inspector
  src/main.cpp
  src/BagReader.cpp
  src/BagInspectorApp.cpp
)
target_include_directories(rosbag_timing_inspector PRIVATE src)
target_link_libraries(rosbag_timing_inspector
  imgui_lib
  implot_lib
  tinyfd_lib
  OpenGL::GL
  glfw
  rosbag2_cpp::rosbag2_cpp
  rosbag2_storage::rosbag2_storage
)

# Pass the rosbag2_storage version as a compile definition so BagReader.cpp
# can use #if to select the correct timestamp field name.
# recv_timestamp was introduced in rosbag2 0.17.0 (Iron / ROS 2 >= Iron).
# Humble shipped 0.15.x and uses the old `time_stamp` field name.
target_compile_definitions(rosbag_timing_inspector PRIVATE
  ROSBAG2_STORAGE_VERSION_MAJOR=${rosbag2_storage_VERSION_MAJOR}
  ROSBAG2_STORAGE_VERSION_MINOR=${rosbag2_storage_VERSION_MINOR}
)

install(TARGETS rosbag_timing_inspector
  DESTINATION bin
)

ament_package()

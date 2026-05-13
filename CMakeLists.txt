cmake_minimum_required(VERSION 3.16)
project(rosbag_timing_inspector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage REQUIRED)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include(FetchContent)

# ---- Dear ImGui ----
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.9
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# ---- ImPlot ----
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        v0.16
)
FetchContent_MakeAvailable(implot)

add_library(implot_lib STATIC
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot_lib PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot_lib PUBLIC imgui_lib)

# ---- tinyfiledialogs ----
FetchContent_Declare(
  tinyfiledialogs
  GIT_REPOSITORY https://git.code.sf.net/p/tinyfiledialogs/code
  GIT_TAG        master
)
FetchContent_MakeAvailable(tinyfiledialogs)

add_library(tinyfd_lib STATIC
  ${tinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c
)
target_include_directories(tinyfd_lib PUBLIC ${tinyfiledialogs_SOURCE_DIR})

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
)
ament_target_dependencies(rosbag_timing_inspector
  rosbag2_cpp
  rosbag2_storage
)

install(TARGETS rosbag_timing_inspector
  DESTINATION bin
)

ament_package()

cmake_minimum_required(VERSION 3.23)
project(tray_control)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 使用 FetchContent 管理第三方依赖
include(FetchContent)

# cxxopts - 命令行选项解析库
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.1.1
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(cxxopts)

# magic_enum - C++枚举反射库
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.3
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(magic_enum)

# ftxui - 终端UI库
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
  GIT_TAG        v5.0.0
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(ftxui)

find_package(sdbus-c++ REQUIRED)
find_package(fmt REQUIRED)

add_library(core STATIC src/StatusNotifierWatcher.cpp src/StatusNotifierItem.cpp)
target_link_libraries(core SDBusCpp::sdbus-c++)

add_executable(tray-show src/tray-show.cpp)
target_link_libraries(tray-show core cxxopts fmt)

add_executable(tray-activate src/tray-activate.cpp)
target_link_libraries(tray-activate core cxxopts fmt)

install(TARGETS tray-show tray-activate)
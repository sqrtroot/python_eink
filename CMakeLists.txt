cmake_minimum_required(VERSION 3.25)
project(python_eink)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED)
find_package(fmt REQUIRED)

add_compile_options("-fpic")

add_library(IT8951_LIB src/IT8951.cpp src/ScreenManager.cpp src/ScsiDriverLinux.cpp src/log.cpp)
target_include_directories(IT8951_LIB PUBLIC include)
set_target_properties(IT8951_LIB PROPERTIES OUTPUT_NAME "IT8951")

target_link_libraries(IT8951_LIB PUBLIC
        fmt::fmt
        opencv_core
        opencv_imgproc
        opencv_imgcodecs)

target_include_directories(IT8951_LIB PUBLIC ${PYTHON_INCLUDE_DIRS})
target_include_directories(IT8951_LIB PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(IT8951_LIB PUBLIC ${OpenCV_INCLUDE_DIRS}/opencv4)


add_subdirectory(python_bindings)
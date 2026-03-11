cmake_minimum_required(VERSION 3.16)

project(BlackHole_GR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenEXR
find_package(OpenEXR REQUIRED)

# Include local headers
include_directories(include)

# Collect all source files
file(GLOB SRC_FILES src/*.cpp)

# Create executable
add_executable(blackhole ${SRC_FILES})

# Link OpenEXR
target_link_libraries(blackhole OpenEXR::OpenEXR)

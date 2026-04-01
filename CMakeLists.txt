cmake_minimum_required(VERSION 3.16)

project(BlackHole_GR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenEXR REQUIRED)

file(GLOB SRC_FILES src/*.cpp)

# 1. Create target FIRST
add_executable(blackhole ${SRC_FILES})

# 2. Then attach include directories
target_include_directories(blackhole PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 3. Link libraries
target_link_libraries(blackhole PRIVATE
    OpenEXR::OpenEXR
    pthread
)

# ---- Sanitizers (comment out for release builds) ----
target_compile_options(blackhole PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
)
target_link_options(blackhole PRIVATE
    -fsanitize=address,undefined
)

cmake_minimum_required(VERSION 3.16)
project(BlackHole_GR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include your headers
include_directories(include)

# Find OpenEXR via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENEXR REQUIRED OpenEXR)

include_directories(${OPENEXR_INCLUDE_DIRS} /usr/include/Imath)

# Executable
add_executable(main src/main.cpp)

# Link OpenEXR libraries + pthread + zlib
target_link_libraries(main ${OPENEXR_LIBRARIES} pthread z)

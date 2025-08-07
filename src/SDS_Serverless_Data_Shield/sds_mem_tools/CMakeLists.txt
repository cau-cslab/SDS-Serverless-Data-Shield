cmake_minimum_required(VERSION 3.10)
project(sds_mem_tools C)

# Enforce C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Locate Python interpreter and development libraries/headers
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Collect all C sources under src/
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Define library target
add_library(sdsmemtools SHARED ${SOURCES})

# Include Python headers for target
target_include_directories(sdsmemtools PRIVATE ${Python3_INCLUDE_DIRS})

# Link against Python library
target_link_libraries(sdsmemtools PRIVATE Python3::Python)

# Rename the output file to be a python extension
set_target_properties(sdsmemtools PROPERTIES
        PREFIX ""
        SUFFIX ".so"
)
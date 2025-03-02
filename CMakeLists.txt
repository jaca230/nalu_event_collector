cmake_minimum_required(VERSION 3.10)

# Project name and version
project(nalu_event_collector_main)

# Set the output directories for the build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Specify include directories
include_directories(include)

# Gather all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create a static library from all source files
add_library(nalu_event_collector_lib STATIC ${SOURCES})

# Create the executable
add_executable(main main.cpp)

# Link the executable with the static library
target_link_libraries(main PRIVATE nalu_event_collector_lib)

# Specify where to install the header files and library
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/nalu_event_collector)

# Install the library into /usr/local/nalu_event_collector/lib
install(TARGETS nalu_event_collector DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# Do not install the executable by default
# Uncomment the following line to install the executable if needed:
# install(TARGETS nalu_event_collector_main DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

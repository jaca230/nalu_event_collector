cmake_minimum_required(VERSION 3.10)

project(nalu_event_collector_main)

# Build output directories (inside your project source root)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Include directories
include_directories(include)

# Collect source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Build static library
add_library(nalu_event_collector STATIC ${SOURCES})

# Build executable
add_executable(main main.cpp)

# Link executable with library and pthread
target_link_libraries(main PRIVATE nalu_event_collector pthread)

# Install rules for headers and library (these affect `make install` only)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/nalu_event_collector)
install(TARGETS nalu_event_collector DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# Uncomment below to install executable on install
# install(TARGETS main DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

cmake_minimum_required(VERSION 3.14)

project(nalu_event_collector VERSION 0.1 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories (optional, useful for local builds)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Create static library target
add_library(nalu_event_collector STATIC ${SOURCES})

# Specify include directories for the library (public so dependents get it)
target_include_directories(nalu_event_collector PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nalu_event_collector>
)

# Set compile options or warnings if needed (optional)
# target_compile_options(nalu_event_collector PRIVATE -Wall -Wextra -Wpedantic)

# Add executable target
add_executable(nalu_event_collector_main main.cpp)

# Link executable to library and pthread
target_link_libraries(nalu_event_collector_main PRIVATE nalu_event_collector pthread)

# Make sure executable also includes headers
target_include_directories(nalu_event_collector_main PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Installation rules

# Install library target and export config for downstream usage
install(TARGETS nalu_event_collector
  EXPORT nalu_event_collectorTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include/nalu_event_collector
)

# Install executable (optional)
install(TARGETS nalu_event_collector_main
  RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/ DESTINATION include/nalu_event_collector)

# Export config files for find_package() support (optional, advanced)
install(EXPORT nalu_event_collectorTargets
  FILE nalu_event_collectorTargets.cmake
  NAMESPACE nalu_event_collector::
  DESTINATION lib/cmake/nalu_event_collector
)

# Optionally create a ConfigVersion.cmake and Config.cmake to support find_package() - can be added later

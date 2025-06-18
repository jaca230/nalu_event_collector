cmake_minimum_required(VERSION 3.14)

project(nalu_event_collector VERSION 0.1 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads package
find_package(Threads REQUIRED)

# Do NOT override global output directories here to avoid conflicts in superprojects

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Create static library target
add_library(nalu_event_collector STATIC ${SOURCES})
set_target_properties(nalu_event_collector PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# Specify include directories (public for downstream usage)
target_include_directories(nalu_event_collector PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nalu_event_collector>
)

# Add executable target for testing/demo purposes
add_executable(nalu_event_collector_main ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# Link executable to library and pthread (via Threads::Threads target)
target_link_libraries(nalu_event_collector_main PRIVATE nalu_event_collector Threads::Threads)

# Include headers for executable target as well
target_include_directories(nalu_event_collector_main PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Installation rules
include(GNUInstallDirs)

install(TARGETS nalu_event_collector
  EXPORT nalu_event_collectorTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nalu_event_collector
)

install(TARGETS nalu_event_collector_main
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nalu_event_collector
)

install(EXPORT nalu_event_collectorTargets
  FILE nalu_event_collectorTargets.cmake
  NAMESPACE nalu_event_collector::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nalu_event_collector
)

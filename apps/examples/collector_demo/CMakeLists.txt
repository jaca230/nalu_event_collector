cmake_minimum_required(VERSION 3.14)

project(collector_demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(USE_LOCAL_NALU_EVENT_COLLECTOR "Build against a local nalu_event_collector source tree" ON)
set(NALU_EVENT_COLLECTOR_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../.."
    CACHE PATH "Path to a local nalu_event_collector source tree")

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/CPM.cmake)

set(CPM_NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann_json version")

if(USE_LOCAL_NALU_EVENT_COLLECTOR AND EXISTS "${NALU_EVENT_COLLECTOR_SOURCE_DIR}/CMakeLists.txt")
  CPMAddPackage(
    NAME nalu_event_collector
    SOURCE_DIR "${NALU_EVENT_COLLECTOR_SOURCE_DIR}"
  )
else()
  find_package(nalu_event_collector REQUIRED)
endif()

CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION ${CPM_NLOHMANN_JSON_VERSION}
)

add_executable(collector_demo main.cpp)
target_link_libraries(collector_demo
  PRIVATE
    nalu_event_collector::nalu_event_collector
    nlohmann_json::nlohmann_json
)

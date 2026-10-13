cmake_minimum_required(VERSION 3.20)
project(HARP LANGUAGES CXX)

# ---------------------------------------------------------------------------
# HARP: Hadron Analysis & Response Performance framework
#
# Layout (one CMake target per package, see cmake/HarpMacros.cmake):
#   Core/         framework kernel      -> libHarpCore.so
#   DataFormats/  event data model      -> libHarpDataFormats.so
#   Fit/          fitting utilities     -> libHarpFit.so
#   Plot/         plotting kernel       -> libHarpPlot.so  (+ plotter plugins)
#   Modules/*/    plugin modules        -> lib/plugins/libHarp<Pkg>Plugin.so
#   bin/          executables           -> bin/harpRun, bin/harpPlot, bin/harpInspect
# ---------------------------------------------------------------------------

set(HARP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(HARP_BINARY_DIR ${CMAKE_BINARY_DIR})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

find_package(ROOT REQUIRED COMPONENTS Core RIO Tree TreePlayer Hist HistPainter Graf Gpad MathCore Physics)
# Follow the C++ standard ROOT was built with (LCG_107a: C++20, system ROOT: C++17).
if(DEFINED ROOT_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH "${HARP_BINARY_DIR}/lib")

set(HARP_ROOT_LIBS ROOT::Core ROOT::RIO ROOT::Tree ROOT::TreePlayer ROOT::Hist ROOT::Graf ROOT::Gpad ROOT::MathCore ROOT::Physics)
set(HARP_WARNINGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)

# nlohmann/json: either an installed package or the header found on the include path.
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
  if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann/json.hpp not found. Source env/setup.sh (LCG view) or install nlohmann-json.")
  endif()
  add_library(nlohmann_json INTERFACE)
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
  target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

include(cmake/HarpMacros.cmake)

add_subdirectory(Core)
add_subdirectory(DataFormats)
add_subdirectory(Fit)
add_subdirectory(Plot)
add_subdirectory(Modules)
add_subdirectory(bin)

message(STATUS "HARP: ROOT ${ROOT_VERSION}, C++${CMAKE_CXX_STANDARD}, build type ${CMAKE_BUILD_TYPE}")

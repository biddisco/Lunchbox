
# Copyright (c) 2012-2013 Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Lunchbox)

#-----------------------------------------------------------------------------
# Global Variable macro
#-----------------------------------------------------------------------------
MACRO (SET_GLOBAL_VARIABLE name value)
  SET (${name} ${value} CACHE INTERNAL "Used to pass variables between directories" FORCE)
  message("${name} SET to ${value}")
ENDMACRO (SET_GLOBAL_VARIABLE)
MACRO (ADD_GLOBAL_VARIABLE name value)
  SET (${name} ${${name}} ${value} CACHE INTERNAL "Used to pass variables between directories" FORCE)
  message("${name} ADDED to ${${name}}")
ENDMACRO (ADD_GLOBAL_VARIABLE)

#-----------------------------------------------------------------------------
# Declare a list of targets we are going to export from this project
# If a sub-project of another, don't change the target export name
#-----------------------------------------------------------------------------
if (NOT PROJECT_EXPORTED_TARGETS)
  SET (PROJECT_EXPORTED_TARGETS "lunchbox-targets")
endif (NOT PROJECT_EXPORTED_TARGETS)

#-----------------------------------------------------------------------------
# To enable projects to compile against a build tree, we export local targets
# using this variable
#-----------------------------------------------------------------------------
if (NOT Lunchbox_EXTERNALLY_CONFIGURED)
  # clear this variable ONLY if we are the root project
  SET_GLOBAL_VARIABLE (PROJECT_EXPORTED_TARGETS_BUILD "")
endif (NOT Lunchbox_EXTERNALLY_CONFIGURED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake
  ${CMAKE_CURRENT_SOURCE_DIR}/CMake/common)
include(.gitexternals)

#include(Buildyard)
if(BUILDYARD_STOP)
  return()
endif()

option(LUNCHBOX_USE_OPENMP "Test for OpenMP support" ON)
option(LUNCHBOX_BUILD_V2_API
  "Enable for pure 2.0 API (breaks compatibility with 1.x API)" OFF)
option(LUNCHBOX_FORCE_OPENMP "Force usage of buggy OpenMP implementation" OFF)
mark_as_advanced(LUNCHBOX_FORCE_OPENMP)

set(RELEASE_VERSION OFF) # OFF or ABI version
set(LAST_RELEASE 1.8.0) # tarball, MacPorts, ...
set(VERSION_MAJOR "1")
set(VERSION_MINOR "9")
set(VERSION_PATCH "0")

include(Common)
#include(GitTargets)
include(UpdateFile)

if(LUNCHBOX_BUILD_V2_API)
  list(APPEND FIND_PACKAGES_DEFINES LUNCHBOX_USE_V2_API)
else()
  list(APPEND FIND_PACKAGES_DEFINES LUNCHBOX_USE_V1_API)
endif()

set(PROJECT_INCLUDE_NAME lunchbox)
include(FindPackages)
set(LUNCHBOX_DEPENDENT_LIBRARIES Boost)

if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(LUNCHBOX_DEFINITIONS ${LUNCHBOX_DEFINITIONS}
  -DLUNCHBOX_BUILD_DIR="${CMAKE_BINARY_DIR}/")
add_definitions(${LUNCHBOX_DEFINITIONS})

if(APPLE)
  # WAR needed when CMAKE_OSX_SYSROOT is set to find /usr/include/turbojpeg.h
  include_directories(SYSTEM /usr/local/../include)
endif()

include(InstallSymlink)
include(pthreads/CMakeLists.txt)

add_subdirectory(lunchbox)
add_subdirectory(tests)
add_subdirectory(doc)

#include(DoxygenRule) # must be after all targets
#include(CPackConfig)
#include(PackageConfig)
#include(GNUModules)

set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
  # http://public.kitware.com/Bug/view.php?id=10179
  "ld: warning: duplicate dylib")
include(CTest)

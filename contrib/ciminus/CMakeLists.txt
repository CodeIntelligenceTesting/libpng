# Copyright (c) 2018-2025 Cosmin Truta
#
# This software is released under the MIT license. For conditions of
# distribution and use, see the LICENSE file part of this package.

cmake_minimum_required(VERSION 3.14)

project(CIMINUS C)

option(CIMINUS_USE_SYSTEM_CI
       "Use the libci build found in the system" OFF)

add_executable(ci2pnm ci2pnm.c)
add_executable(pnm2ci pnm2ci.c)

if(CIMINUS_USE_SYSTEM_CI)
    # Use the system libci.
    find_package(CI REQUIRED)
    target_link_libraries(ci2pnm PRIVATE CI::CI)
    target_link_libraries(pnm2ci PRIVATE CI::CI)
else()
    # Build and use the internal libci.
    # Configure libci for static linking, to produce single-file executables.
    set(CI_STATIC ON
        CACHE STRING "Build the internal libci as a static library" FORCE)
    set(CI_SHARED OFF
        CACHE STRING "Build the internal libci as a shared library" FORCE)
    set(CI_FRAMEWORK OFF
        CACHE STRING "Build the internal libci as a framework bundle" FORCE)
    add_subdirectory(../.. libci)
    target_include_directories(ci2pnm PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libci>"
    )
    target_include_directories(pnm2ci PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libci>"
    )
    target_link_libraries(ci2pnm PRIVATE ci_static)
    target_link_libraries(pnm2ci PRIVATE ci_static)
endif()

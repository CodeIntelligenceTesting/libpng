# CIConfig.cmake
# CMake config file compatible with the FindCI module.

# Copyright (c) 2024 Cosmin Truta
# Written by Benjamin Buch, 2024
#
# Use, modification and distribution are subject to
# the same licensing terms and conditions as libci.
# Please see the copyright notice in ci.h or visit
# http://libci.org/pub/ci/src/libci-LICENSE.txt
#
# SPDX-License-Identifier: libci-2.0

include(CMakeFindDependencyMacro)

find_dependency(ZLIB REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/CITargets.cmake")

if(NOT TARGET CI::CI)
  if(TARGET CI::ci_shared)
    add_library(CI::CI INTERFACE IMPORTED)
    target_link_libraries(CI::CI INTERFACE CI::ci_shared)
  elseif(TARGET CI::ci_static)
    add_library(CI::CI INTERFACE IMPORTED)
    target_link_libraries(CI::CI INTERFACE CI::ci_static)
  endif()
endif()

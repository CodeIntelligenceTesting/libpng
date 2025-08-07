# CMakeLists.txt - CMake lists for libci
#
# Copyright (c) 2018-2025 Cosmin Truta
# Copyright (c) 2007-2018 Glenn Randers-Pehrson
# Originally written by Christian Ehrlicher, 2007
#
# Use, modification and distribution are subject to
# the same licensing terms and conditions as libci.
# Please see the copyright notice in ci.h or visit
# http://libci.org/pub/ci/src/libci-LICENSE.txt
#
# For copyright and licensing purposes, please see
# the accompanying file scripts/cmake/AUTHORS.md
#
# SPDX-License-Identifier: libci-2.0

cmake_minimum_required(VERSION 3.14...4.0)

set(CILIB_MAJOR 1)
set(CILIB_MINOR 6)
set(CILIB_REVISION 50)
#set(CILIB_SUBREVISION 0)
set(CILIB_SUBREVISION "git")
set(CILIB_VERSION ${CILIB_MAJOR}.${CILIB_MINOR}.${CILIB_REVISION})
set(CILIB_ABI_VERSION ${CILIB_MAJOR}${CILIB_MINOR})
set(CILIB_SHARED_VERSION ${CILIB_ABI_VERSION}.${CILIB_REVISION}.${CILIB_SUBREVISION})

project(libci
        VERSION ${CILIB_VERSION}
        LANGUAGES C ASM)

enable_language(CXX)
find_package(cifuzz NO_SYSTEM_ENVIRONMENT_PATH)
enable_fuzz_testing()


include(CheckCSourceCompiles)
include(GNUInstallDirs)

# Allow the users to specify an application-specific API prefix for libci
# vendoring purposes. A standard libci build should have no such prefix.
set(CI_PREFIX
    ""
    CACHE STRING "Prefix to prepend to the API function names")

# Allow the users to override the postfix appended to debug library file names.
# Previously, we used to set CMAKE_DEBUG_POSTFIX globally. That variable should
# not be cached, however, because doing so would affect all projects processed
# after libci, in unexpected and undesirable ways.
set(CI_DEBUG_POSTFIX
    "d"
    CACHE STRING "Postfix to append to library file names under the Debug configuration")

# Allow the users to import their own extra configuration settings.
# Those settings can be either passed via DFA_XTRA if they are in DFA form
# (such as "ciusr.dfa"), or via CI_LIBCONF_HEADER if they are in prebuilt
# header file form (such as "scripts/cilibconf.h.prebuilt"), but not both.
# For platforms such as Android or iOS, or in certain cross-platform build
# scenarios, having a valid CI_LIBCONF_HEADER is mandatory.
set(DFA_XTRA
    ""
    CACHE FILEPATH "DFA file containing customized build configuration settings for libci")
set(CI_LIBCONF_HEADER
    ""
    CACHE FILEPATH "C header file containing customized build configuration settings for libci")
set(CI_LIBCONF_HEADER_PREBUILT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cilibconf.h.prebuilt")
if(ANDROID OR IOS)
  set(CI_LIBCONF_HEADER "${CI_LIBCONF_HEADER_PREBUILT}")
endif()
if((NOT DFA_XTRA STREQUAL "") AND (NOT CI_LIBCONF_HEADER STREQUAL ""))
  message(SEND_ERROR "The options DFA_XTRA=\"${DFA_XTRA}\" "
                     "and CI_LIBCONF_HEADER=\"${CI_LIBCONF_HEADER}\" "
                     "are mutually exclusive")
endif()

# Allow the users to switch on/off various library build types.
option(CI_SHARED "Build libci as a shared library" ON)
option(CI_STATIC "Build libci as a static library" ON)
if(APPLE)
  option(CI_FRAMEWORK "Build libci as a framework bundle" ON)
else()
  option(CI_FRAMEWORK "Build libci as a framework bundle (not available on this platform)" OFF)
endif()
if(NOT APPLE AND CI_FRAMEWORK)
  message(SEND_ERROR "The option CI_FRAMEWORK should not be set on this platform")
endif()

# Allow the users to switch on/off the auxiliary build and test artifacts.
# These artifacts are NOT part of libci proper, and are subject to change
# at any time.
option(CI_TESTS "Build the libci tests" ON)

# Same as above, but for the third-party tools.
# Although these tools are targetted at development environments only,
# the users are allowed to override the option to build by default.
if(ANDROID OR IOS)
  option(CI_TOOLS "Build the libci tools" OFF)
else()
  option(CI_TOOLS "Build the libci tools" ON)
endif()

# Maintain backwards compatibility with the deprecated option CI_EXECUTABLES.
option(CI_EXECUTABLES "[Deprecated; please use CI_TOOLS]" ON)
if(NOT CI_EXECUTABLES)
  message(DEPRECATION "The option CI_EXECUTABLES has been deprecated in favour of CI_TOOLS")
  if(CI_TOOLS)
    message(AUTHOR_WARNING "Setting CI_TOOLS to ${CI_EXECUTABLES}, "
                           "to stay compatible with CI_EXECUTABLES")
    set(CI_TOOLS "${CI_EXECUTABLES}")
  endif()
endif()

# Allow the users to switch on/off the use of hardware (SIMD) optimized code.
option(CI_HARDWARE_OPTIMIZATIONS "Enable hardware optimizations" ON)

# Initialize and show the target architecture variable CI_TARGET_ARCHITECTURE.
#
# NOTE:
# On macOS, CMake sets CMAKE_SYSTEM_PROCESSOR to either "x86_64" or "arm64",
# based upon the OS architecture, not the target architecture. As such, we need
# to check CMAKE_OSX_ARCHITECTURES to identify which hardware-specific flags to
# enable. Note that this will fail if you attempt to build a universal binary
# in a single CMake invocation.
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  string(TOLOWER "${CMAKE_OSX_ARCHITECTURES}" CI_TARGET_ARCHITECTURE)
else()
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CI_TARGET_ARCHITECTURE)
endif()
message(STATUS "Building for target architecture: ${CI_TARGET_ARCHITECTURE}")

# Allow the users to specify a custom location of zlib.
# With CMake 3.12 and newer, this option is no longer necessary.
option(CI_BUILD_ZLIB "[Deprecated; please use ZLIB_ROOT]" OFF)
if(CI_BUILD_ZLIB)
  if("x${ZLIB_ROOT}" STREQUAL "x")
    message(SEND_ERROR "The option CI_BUILD_ZLIB=${CI_BUILD_ZLIB} is no longer supported; "
                       "please use ZLIB_ROOT instead")
  else()
    message(SEND_ERROR "The option CI_BUILD_ZLIB=${CI_BUILD_ZLIB} is no longer supported; "
                       "using ZLIB_ROOT=\"${ZLIB_ROOT}\"")
  endif()
endif()

# Find the zlib library.
find_package(ZLIB REQUIRED)

# Find the math library (where available).
if(UNIX
   AND NOT (APPLE OR BEOS OR HAIKU)
   AND NOT EMSCRIPTEN)
  find_library(M_LIBRARY m)
  if(M_LIBRARY)
    set(M_LIBRARY m)
  else()
    set(M_LIBRARY "")
  endif()
else()
  # libm is not available or not needed.
endif()

# Silence function deprecation warnings on the Windows compilers that might
# use the MSVC Runtime library headers.
if(WIN32 AND (CMAKE_C_COMPILER_ID MATCHES "MSVC|Intel|Clang"))
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

if(CI_HARDWARE_OPTIMIZATIONS)

  # Set definitions and sources for ARM.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(arm|aarch)")
    if(CI_TARGET_ARCHITECTURE MATCHES "^(arm64|aarch64)")
      set(CI_ARM_NEON_POSSIBLE_VALUES on off)
      set(CI_ARM_NEON
          "on"
          CACHE STRING "Enable ARM NEON optimizations: on|off; on is default")
    else()
      set(CI_ARM_NEON_POSSIBLE_VALUES check on off)
      set(CI_ARM_NEON
          "off"
          CACHE STRING "Enable ARM NEON optimizations: check|on|off; off is default")
    endif()
    set_property(CACHE CI_ARM_NEON
                 PROPERTY STRINGS ${CI_ARM_NEON_POSSIBLE_VALUES})
    list(FIND CI_ARM_NEON_POSSIBLE_VALUES ${CI_ARM_NEON} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "CI_ARM_NEON must be one of [${CI_ARM_NEON_POSSIBLE_VALUES}]")
    elseif(NOT CI_ARM_NEON STREQUAL "off")
      set(libci_arm_sources
          arm/arm_init.c
          arm/filter_neon_intrinsics.c
          arm/palette_neon_intrinsics.c)
      if(CI_ARM_NEON STREQUAL "on")
        add_definitions(-DCI_ARM_NEON_OPT=2)
      elseif(CI_ARM_NEON STREQUAL "check")
        add_definitions(-DCI_ARM_NEON_CHECK_SUPPORTED)
      endif()
    else()
      add_definitions(-DCI_ARM_NEON_OPT=0)
    endif()
  endif()

  # Set definitions and sources for PowerPC.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(powerpc|ppc64)")
    set(CI_POWERPC_VSX_POSSIBLE_VALUES on off)
    set(CI_POWERPC_VSX
        "on"
        CACHE STRING "Enable POWERPC VSX optimizations: on|off; on is default")
    set_property(CACHE CI_POWERPC_VSX
                 PROPERTY STRINGS ${CI_POWERPC_VSX_POSSIBLE_VALUES})
    list(FIND CI_POWERPC_VSX_POSSIBLE_VALUES ${CI_POWERPC_VSX} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "CI_POWERPC_VSX must be one of [${CI_POWERPC_VSX_POSSIBLE_VALUES}]")
    elseif(NOT CI_POWERPC_VSX STREQUAL "off")
      set(libci_powerpc_sources
          powerpc/powerpc_init.c
          powerpc/filter_vsx_intrinsics.c)
      if(CI_POWERPC_VSX STREQUAL "on")
        add_definitions(-DCI_POWERPC_VSX_OPT=2)
      endif()
    else()
      add_definitions(-DCI_POWERPC_VSX_OPT=0)
    endif()
  endif()

  # Set definitions and sources for Intel.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(i[3-6]86|x86|amd64)")
    set(CI_INTEL_SSE_POSSIBLE_VALUES on off)
    set(CI_INTEL_SSE
        "on"
        CACHE STRING "Enable INTEL_SSE optimizations: on|off; on is default")
    set_property(CACHE CI_INTEL_SSE
                 PROPERTY STRINGS ${CI_INTEL_SSE_POSSIBLE_VALUES})
    list(FIND CI_INTEL_SSE_POSSIBLE_VALUES ${CI_INTEL_SSE} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "CI_INTEL_SSE must be one of [${CI_INTEL_SSE_POSSIBLE_VALUES}]")
    elseif(NOT CI_INTEL_SSE STREQUAL "off")
      set(libci_intel_sources
          intel/intel_init.c
          intel/filter_sse2_intrinsics.c)
      if(CI_INTEL_SSE STREQUAL "on")
        add_definitions(-DCI_INTEL_SSE_OPT=1)
      endif()
    else()
      add_definitions(-DCI_INTEL_SSE_OPT=0)
    endif()
  endif()

  # Set definitions and sources for MIPS.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(mipsel|mips64el)")
    set(CI_MIPS_MSA_POSSIBLE_VALUES on off)
    set(CI_MIPS_MSA
        "on"
        CACHE STRING "Enable MIPS_MSA optimizations: on|off; on is default")
    set_property(CACHE CI_MIPS_MSA
                 PROPERTY STRINGS ${CI_MIPS_MSA_POSSIBLE_VALUES})
    list(FIND CI_MIPS_MSA_POSSIBLE_VALUES ${CI_MIPS_MSA} index_msa)
    if(index_msa EQUAL -1)
      message(FATAL_ERROR "CI_MIPS_MSA must be one of [${CI_MIPS_MSA_POSSIBLE_VALUES}]")
    endif()

    set(CI_MIPS_MMI_POSSIBLE_VALUES on off)
    set(CI_MIPS_MMI
        "on"
        CACHE STRING "Enable MIPS_MMI optimizations: on|off; on is default")
    set_property(CACHE CI_MIPS_MMI
                 PROPERTY STRINGS ${CI_MIPS_MMI_POSSIBLE_VALUES})
    list(FIND CI_MIPS_MMI_POSSIBLE_VALUES ${CI_MIPS_MMI} index_mmi)
    if(index_mmi EQUAL -1)
      message(FATAL_ERROR "CI_MIPS_MMI must be one of [${CI_MIPS_MMI_POSSIBLE_VALUES}]")
    endif()

    if(CI_MIPS_MSA STREQUAL "on" AND CI_MIPS_MMI STREQUAL "on")
      set(libci_mips_sources
          mips/mips_init.c
          mips/filter_msa_intrinsics.c
          mips/filter_mmi_inline_assembly.c)
      add_definitions(-DCI_MIPS_MSA_OPT=2)
      add_definitions(-DCI_MIPS_MMI_OPT=1)
    elseif(CI_MIPS_MSA STREQUAL "on")
      set(libci_mips_sources
          mips/mips_init.c
          mips/filter_msa_intrinsics.c)
      add_definitions(-DCI_MIPS_MSA_OPT=2)
      add_definitions(-DCI_MIPS_MMI_OPT=0)
    elseif(CI_MIPS_MMI STREQUAL "on")
      set(libci_mips_sources
          mips/mips_init.c
          mips/filter_mmi_inline_assembly.c)
      add_definitions(-DCI_MIPS_MSA_OPT=0)
      add_definitions(-DCI_MIPS_MMI_OPT=1)
    else()
      add_definitions(-DCI_MIPS_MSA_OPT=0)
      add_definitions(-DCI_MIPS_MMI_OPT=0)
    endif()
  endif()

  # Set definitions and sources for LoongArch.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(loongarch)")
    include(CheckCCompilerFlag)
    set(CI_LOONGARCH_LSX_POSSIBLE_VALUES on off)
    set(CI_LOONGARCH_LSX
        "on"
        CACHE STRING "Enable LOONGARCH_LSX optimizations: on|off; on is default")
    set_property(CACHE CI_LOONGARCH_LSX
                 PROPERTY STRINGS ${CI_LOONGARCH_LSX_POSSIBLE_VALUES})
    list(FIND CI_LOONGARCH_LSX_POSSIBLE_VALUES ${CI_LOONGARCH_LSX} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "CI_LOONGARCH_LSX must be one of [${CI_LOONGARCH_LSX_POSSIBLE_VALUES}]")
    elseif(NOT CI_LOONGARCH_LSX STREQUAL "off")
      check_c_compiler_flag("-mlsx" COMPILER_SUPPORTS_LSX)
      if(COMPILER_SUPPORTS_LSX)
        set(libci_loongarch_sources
            loongarch/loongarch_lsx_init.c
            loongarch/filter_lsx_intrinsics.c)
        set_source_files_properties(${libci_loongarch_sources}
                                    PROPERTIES COMPILE_FLAGS "-mlsx")
        add_definitions(-DCI_LOONGARCH_LSX_OPT=1)
      else()
        message(FATAL_ERROR "This compiler does not support the -mlsx option")
      endif()
    else()
      add_definitions(-DCI_LOONGARCH_LSX_OPT=0)
    endif()
  endif()

  # Set definitions and sources for RISC-V.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(riscv)")
    include(CheckCCompilerFlag)
    set(CI_RISCV_RVV_POSSIBLE_VALUES check on off)
    set(CI_RISCV_RVV "off"
      CACHE STRING "Enable RISC-V Vector optimizations: check|on|off; off is default")
    set_property(CACHE CI_RISCV_RVV
                PROPERTY STRINGS ${CI_RISCV_RVV_POSSIBLE_VALUES})
    list(FIND CI_RISCV_RVV_POSSIBLE_VALUES ${CI_RISCV_RVV} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "CI_RISCV_RVV must be one of [${CI_RISCV_RVV_POSSIBLE_VALUES}]")
    elseif(NOT CI_RISCV_RVV STREQUAL "off")

      set(_SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
      set(CMAKE_REQUIRED_FLAGS "-march=rv64gv1p0")

      check_c_source_compiles("
        #include <riscv_vector.h>
        #include <asm/hwcap.h>
        #ifndef COMPAT_HWCAP_ISA_V /* added in linux-6.5 */
        #error \"COMPAT_HWCAP_ISA_V is not available\"
        #endif
        int main() {
          const float src[] = { 0.0f, 0.0f, 0.0f, 0.0f };
          uint64_t ptr[2] = {0x0908060504020100, 0xFFFFFFFF0E0D0C0A};
          vuint8m1_t a = __riscv_vreinterpret_v_u64m1_u8m1(__riscv_vle64_v_u64m1(ptr, 2));
          vfloat32m1_t val = __riscv_vle32_v_f32m1((const float*)(src), 4);
          return (int)__riscv_vfmv_f_s_f32m1_f32(val);
      }" COMPILER_SUPPORTS_RVV)

      set(CMAKE_REQUIRED_FLAGS ${_SAVED_CMAKE_REQUIRED_FLAGS})

      if(NOT COMPILER_SUPPORTS_RVV)
        message(FATAL_ERROR "Compiler does not support RISC-V Vector extension or its unable to detect it")
      endif()
      set(libci_riscv_sources
          riscv/filter_rvv_intrinsics.c
          riscv/riscv_init.c)
      if(CI_RISCV_RVV STREQUAL "on")
        add_definitions(-DCI_RISCV_RVV_OPT=2)
      elseif(CI_RISCV_RVV STREQUAL "check")
        add_definitions(-DCI_RISCV_RVV_CHECK_SUPPORTED)
        add_definitions(-DCI_RISCV_RVV_OPT=1)
      endif()
    else()
      add_definitions(-DCI_RISCV_RVV_OPT=0)
    endif()
  endif()

else(CI_HARDWARE_OPTIMIZATIONS)

  # Set definitions and sources for ARM.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(arm|aarch)")
    add_definitions(-DCI_ARM_NEON_OPT=0)
  endif()

  # Set definitions and sources for PowerPC.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(powerpc|ppc64)")
    add_definitions(-DCI_POWERPC_VSX_OPT=0)
  endif()

  # Set definitions and sources for Intel.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(i[3-6]86|x86|amd64)")
    add_definitions(-DCI_INTEL_SSE_OPT=0)
  endif()

  # Set definitions and sources for MIPS.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(mipsel|mips64el)")
    add_definitions(-DCI_MIPS_MSA_OPT=0)
  endif()

  # Set definitions and sources for LoongArch.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(loongarch)")
    add_definitions(-DCI_LOONGARCH_LSX_OPT=0)
  endif()

  # Set definitions and sources for RISC-V.
  if(CI_TARGET_ARCHITECTURE MATCHES "^(riscv)")
    add_definitions(-DCI_RISCV_RVV_OPT=0)
  endif()

endif(CI_HARDWARE_OPTIMIZATIONS)

option(ld-version-script "Enable linker version script" ON)
if(ld-version-script AND NOT (ANDROID OR APPLE))
  # Check if LD supports linker scripts.
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map" "
VERS_1 { global: sym1; local: *; };
VERS_2 { global: sym2; main; } VERS_1;
")
  set(_SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  if(NOT CMAKE_HOST_SOLARIS)
    # Avoid using CMAKE_SHARED_LIBRARY_C_FLAGS in version script checks on
    # Solaris, because of an incompatibility with the Solaris link editor.
    list(APPEND CMAKE_REQUIRED_FLAGS ${CMAKE_SHARED_LIBRARY_C_FLAGS})
  endif()
  list(APPEND CMAKE_REQUIRED_FLAGS
              "-Wl,--version-script='${CMAKE_CURRENT_BINARY_DIR}/conftest.map'")
  check_c_source_compiles("
void sym1(void) {}
void sym2(void) {}
int main(void) { return 0; }
" HAVE_LD_VERSION_SCRIPT)
  if(NOT HAVE_LD_VERSION_SCRIPT)
    set(CMAKE_REQUIRED_FLAGS ${_SAVED_CMAKE_REQUIRED_FLAGS})
    if(NOT CMAKE_HOST_SOLARIS)
      # Again, avoid using CMAKE_SHARED_LIBRARY_C_FLAGS in version script
      # checks on Solaris.
      list(APPEND CMAKE_REQUIRED_FLAGS ${CMAKE_SHARED_LIBRARY_C_FLAGS})
    endif()
    list(APPEND CMAKE_REQUIRED_FLAGS "-Wl,-M -Wl,${CMAKE_CURRENT_BINARY_DIR}/conftest.map")
    check_c_source_compiles("
void sym1(void) {}
void sym2(void) {}
int main(void) { return 0; }
" HAVE_SOLARIS_LD_VERSION_SCRIPT)
  endif()
  set(CMAKE_REQUIRED_FLAGS ${_SAVED_CMAKE_REQUIRED_FLAGS})
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map")
endif()

if(CI_LIBCONF_HEADER STREQUAL "")
  # No custom configuration header file has been specified, so we build it
  # from our DFA files and (optionally) out of the user-supplied DFA file.
  # Find an AWK language processor.
  # Start with specific AWK implementations like gawk and nawk, which are
  # known to work with our scripts, then fall back to the system awk.
  find_program(AWK NAMES gawk nawk awk)
  if(AWK)
    message(STATUS "Found AWK program: ${AWK}")
  else()
    message(STATUS "Could not find an AWK-compatible program")
  endif()
endif()

# Include the internal module CICheckLibconf.cmake
include("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/CICheckLibconf.cmake")

if(NOT CI_LIBCONF_HEADER STREQUAL "")
  # Configure libci with the user-defined cilibconf.h file.
  ci_check_libconf(HEADER "${CI_LIBCONF_HEADER}")
  configure_file("${CI_LIBCONF_HEADER}"
                 "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h"
                 @ONLY)
  add_custom_target(ci_genfiles)
elseif(NOT AWK)
  # No AWK program available to generate cilibconf.h.
  # Configure libci with cilibconf.h.prebuilt.
  ci_check_libconf(HEADER "${CI_LIBCONF_HEADER_PREBUILT}")
  configure_file("${CI_LIBCONF_HEADER_PREBUILT}"
                 "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h"
                 @ONLY)
  add_custom_target(ci_genfiles)
else()
  ci_check_libconf(DFA_XTRA "${DFA_XTRA}")

  # Include the internal module CIGenConfig.cmake
  include("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/CIGenConfig.cmake")

  # Work around a limitation of various Windows AWK programs that are
  # unable to process CRLF-terminated AWK scripts.
  # Copy these AWK scripts to a temporary location, converting their
  # line endings from Windows (CRLF) to Unix (LF) at the destination.
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/checksym.awk"
                 "${CMAKE_CURRENT_BINARY_DIR}/scripts/checksym.awk"
                 @ONLY
                 NEWLINE_STYLE LF)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/options.awk"
                 "${CMAKE_CURRENT_BINARY_DIR}/scripts/options.awk"
                 @ONLY
                 NEWLINE_STYLE LF)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/dfn.awk"
                 "${CMAKE_CURRENT_BINARY_DIR}/scripts/dfn.awk"
                 @ONLY
                 NEWLINE_STYLE LF)

  # Generate scripts/cilibconf.h
  generate_source(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/cilibconf.c"
                  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cilibconf.dfa"
                          "${CMAKE_CURRENT_BINARY_DIR}/scripts/options.awk"
                          "${CMAKE_CURRENT_SOURCE_DIR}/ciconf.h")

  # Generate cilibconf.c
  generate_source(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.c"
                  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cilibconf.dfa"
                          "${CMAKE_CURRENT_BINARY_DIR}/scripts/options.awk"
                          "${CMAKE_CURRENT_SOURCE_DIR}/ciconf.h")

  if(CI_PREFIX)
    set(CILIBCONF_H_EXTRA_DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/scripts/prefix.out"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/macro.lst")
    set(CIPREFIX_H_EXTRA_DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/scripts/intprefix.out")
  endif()

  generate_out(INPUT "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.out")

  # Generate cilibconf.h
  generate_source(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h"
                  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.out"
                          ${CILIBCONF_H_EXTRA_DEPENDS})

  generate_out(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/intprefix.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/intprefix.out"
               DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h")

  generate_out(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prefix.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/prefix.out"
               DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ci.h"
                       "${CMAKE_CURRENT_SOURCE_DIR}/ciconf.h"
                       "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.out")

  # Generate ciprefix.h
  generate_source(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ciprefix.h"
                  DEPENDS ${CIPREFIX_H_EXTRA_DEPENDS})

  generate_out(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sym.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/sym.out"
               DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h")

  generate_out(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/symbols.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/symbols.out"
               DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ci.h"
                       "${CMAKE_CURRENT_SOURCE_DIR}/ciconf.h"
                       "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cilibconf.h.prebuilt")

  generate_out(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/vers.c"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/vers.out"
               DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ci.h"
                       "${CMAKE_CURRENT_SOURCE_DIR}/ciconf.h"
                       "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h")

  generate_chk(INPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/symbols.out"
               OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/symbols.chk"
               DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/scripts/checksym.awk"
                       "${CMAKE_CURRENT_SOURCE_DIR}/scripts/symbols.def")

  generate_copy(INPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/sym.out"
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libci.sym")

  generate_copy(INPUT "${CMAKE_CURRENT_BINARY_DIR}/scripts/vers.out"
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libci.vers")

  add_custom_target(ci_genprebuilt
                    COMMAND "${CMAKE_COMMAND}"
                            "-DOUTPUT=scripts/cilibconf.h.prebuilt"
                            -P "${CMAKE_CURRENT_BINARY_DIR}/scripts/cmake/gensrc.cmake"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

  # A single target handles generation of all generated files.
  add_custom_target(ci_genfiles
                    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libci.sym"
                            "${CMAKE_CURRENT_BINARY_DIR}/libci.vers"
                            "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.c"
                            "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h"
                            "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.out"
                            "${CMAKE_CURRENT_BINARY_DIR}/ciprefix.h"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/intprefix.out"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/cilibconf.c"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/prefix.out"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/sym.out"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/symbols.chk"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/symbols.out"
                            "${CMAKE_CURRENT_BINARY_DIR}/scripts/vers.out")
endif()

# List the source code files.
set(libci_public_hdrs
    ci.h
    ciconf.h
    "${CMAKE_CURRENT_BINARY_DIR}/cilibconf.h"
)
set(libci_private_hdrs
    cipriv.h
    cidebug.h
    ciinfo.h
    cistruct.h
)
if(AWK)
  list(APPEND libci_private_hdrs "${CMAKE_CURRENT_BINARY_DIR}/ciprefix.h")
endif()
set(libci_sources
    ${libci_public_hdrs}
    ${libci_private_hdrs}
    ci.c
    cierror.c
    ciget.c
    cimem.c
    cipread.c
    ciread.c
    cirio.c
    cirtran.c
    cirutil.c
    ciset.c
    citrans.c
    ciwio.c
    ciwrite.c
    ciwtran.c
    ciwutil.c
    ${libci_arm_sources}
    ${libci_intel_sources}
    ${libci_mips_sources}
    ${libci_powerpc_sources}
    ${libci_loongarch_sources}
    ${libci_riscv_sources}
)

set(citest_sources
    citest.c
)
set(civalid_sources
    contrib/libtests/civalid.c
)
set(cistest_sources
    contrib/libtests/cistest.c
)
set(ciunknown_sources
    contrib/libtests/ciunknown.c
)
set(ciimage_sources
    contrib/libtests/ciimage.c
)
set(cifix_sources
    contrib/tools/cifix.c
)
set(ci_fix_itxt_sources
    contrib/tools/ci-fix-itxt.c
)

# Now build our targets.

# Initialize the list of libci library targets.
set(CI_LIBRARY_TARGETS "")

# Initialize the libci library file names.
if(UNIX
   OR (WIN32 AND NOT CMAKE_SHARED_LIBRARY_PREFIX STREQUAL "")
   OR (WIN32 AND NOT CMAKE_STATIC_LIBRARY_PREFIX STREQUAL ""))
  # We are on a Unix or Unix-like toolchain like the GNU toolchain on Windows.
  # Library file names are expected to have an implicit prefix such as "lib".
  # Let CMake prepend and append its usual prefixes and suffixes by default.
  set(CI_SHARED_OUTPUT_NAME "ci${CILIB_ABI_VERSION}")
  set(CI_STATIC_OUTPUT_NAME "ci${CILIB_ABI_VERSION}")
else()
  # We are, most likely, on a Windows toolchain like MSVC, Clang on Windows,
  # Borland/Embarcadero, etc. We need to specify the "libci" name explicitly.
  # We also need to use a custom suffix, in order to distinguish between the
  # shared import library name and the static library name.
  set(CI_SHARED_OUTPUT_NAME "libci${CILIB_ABI_VERSION}")
  set(CI_STATIC_OUTPUT_NAME "libci${CILIB_ABI_VERSION}_static")
endif()

if(CI_SHARED)
  add_library(ci_shared SHARED ${libci_sources})
  add_dependencies(ci_shared ci_genfiles)
  list(APPEND CI_LIBRARY_TARGETS ci_shared)
  set_target_properties(ci_shared
                        PROPERTIES OUTPUT_NAME "${CI_SHARED_OUTPUT_NAME}"
                                   DEBUG_POSTFIX "${CI_DEBUG_POSTFIX}"
                                   VERSION "${CILIB_SHARED_VERSION}"
                                   SOVERSION "${CILIB_ABI_VERSION}")
  if(UNIX AND AWK)
    if(HAVE_LD_VERSION_SCRIPT)
      set_target_properties(ci_shared
                            PROPERTIES LINK_FLAGS "-Wl,--version-script='${CMAKE_CURRENT_BINARY_DIR}/libci.vers'")
    elseif(HAVE_SOLARIS_LD_VERSION_SCRIPT)
      set_target_properties(ci_shared
                            PROPERTIES LINK_FLAGS "-Wl,-M -Wl,'${CMAKE_CURRENT_BINARY_DIR}/libci.vers'")
    endif()
  endif()
  if(APPLE)
    # Avoid CMake's implicit compile definition "ci_shared_EXPORTS".
    set_target_properties(ci_shared
                          PROPERTIES DEFINE_SYMBOL "")
  elseif(WIN32)
    # Use the explicit compile definition "CI_BUILD_DLL" for Windows DLLs.
    set_target_properties(ci_shared
                          PROPERTIES DEFINE_SYMBOL CI_BUILD_DLL)
  endif()
  target_include_directories(ci_shared
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
  target_include_directories(ci_shared
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
  target_include_directories(ci_shared
                             SYSTEM
                             INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libci${CILIB_ABI_VERSION}>")
  target_link_libraries(ci_shared
                        PUBLIC ZLIB::ZLIB ${M_LIBRARY})
endif()

if(CI_STATIC)
  add_library(ci_static STATIC ${libci_sources})
  add_dependencies(ci_static ci_genfiles)
  list(APPEND CI_LIBRARY_TARGETS ci_static)
  set_target_properties(ci_static
                        PROPERTIES OUTPUT_NAME "${CI_STATIC_OUTPUT_NAME}"
                                   DEBUG_POSTFIX "${CI_DEBUG_POSTFIX}")
  target_include_directories(ci_static
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
  target_include_directories(ci_static
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
  target_include_directories(ci_static
                             SYSTEM
                             INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libci${CILIB_ABI_VERSION}>")
  target_link_libraries(ci_static
                        PUBLIC ZLIB::ZLIB ${M_LIBRARY})
endif()

if(CI_FRAMEWORK AND NOT APPLE)
  message(AUTHOR_WARNING "Setting CI_FRAMEWORK to OFF, as it only applies to Apple systems")
  set(CI_FRAMEWORK OFF)
endif()

if(CI_FRAMEWORK)
  add_library(ci_framework SHARED ${libci_sources})
  add_dependencies(ci_framework ci_genfiles)
  list(APPEND CI_LIBRARY_TARGETS ci_framework)
  set_target_properties(ci_framework
                        PROPERTIES FRAMEWORK TRUE
                                   FRAMEWORK_VERSION "${CILIB_VERSION}"
                                   MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${CILIB_MAJOR}.${CILIB_MINOR}"
                                   MACOSX_FRAMEWORK_BUNDLE_VERSION "${CILIB_VERSION}"
                                   MACOSX_FRAMEWORK_IDENTIFIER "org.libci.libci"
                                   XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
                                   PUBLIC_HEADER "${libci_public_hdrs}"
                                   OUTPUT_NAME "ci"
                                   DEBUG_POSTFIX "${CI_DEBUG_POSTFIX}")
  # Avoid CMake's implicit compile definition "-Dci_framework_EXPORTS".
  set_target_properties(ci_framework
                        PROPERTIES DEFINE_SYMBOL "")
  target_include_directories(ci_framework
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
  target_include_directories(ci_framework
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
  target_include_directories(ci_framework
                             SYSTEM
                             INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libci${CILIB_ABI_VERSION}>")
  target_link_libraries(ci_framework
                        PUBLIC ZLIB::ZLIB ${M_LIBRARY})
endif()

if(NOT CI_LIBRARY_TARGETS)
  message(SEND_ERROR "No library variant selected to build. "
                     "Please enable at least one of the following options: "
                     "CI_SHARED, CI_STATIC, CI_FRAMEWORK")
endif()

if(CI_TESTS AND CI_SHARED)
  enable_testing()

  # Include the internal module CITest.cmake
  include("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/CITest.cmake")

  # Find test CI files by globbing, but sort lists to ensure
  # consistency between different filesystems.
  file(GLOB CISUITE_CIS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cisuite/*.ci")
  list(SORT CISUITE_CIS)
  file(GLOB TEST_CIS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/testcis/*.ci")
  list(SORT TEST_CIS)
  file(GLOB TEST_CI3_CIS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/testcis/ci-3/*.ci")
  list(SORT TEST_CI3_CIS)

  set(CITEST_CI "${CMAKE_CURRENT_SOURCE_DIR}/citest.ci")

  add_executable(citest ${citest_sources})
  target_link_libraries(citest
                        PRIVATE ci_shared)

  ci_add_test(NAME citest
               COMMAND citest
               FILES "${CITEST_CI}")

  ci_add_test(NAME citest-ci-3
               COMMAND citest
               FILES "${TEST_CI3_CIS}")

  add_executable(civalid ${civalid_sources})
  target_link_libraries(civalid
                        PRIVATE ci_shared)

  ci_add_test(NAME civalid-gamma-16-to-8
               COMMAND civalid
               OPTIONS --gamma-16-to-8)
  ci_add_test(NAME civalid-gamma-alpha-mode
               COMMAND civalid
               OPTIONS --gamma-alpha-mode)
  ci_add_test(NAME civalid-gamma-background
               COMMAND civalid
               OPTIONS --gamma-background)
  ci_add_test(NAME civalid-gamma-expand16-alpha-mode
               COMMAND civalid
               OPTIONS --gamma-alpha-mode --expand16)
  ci_add_test(NAME civalid-gamma-expand16-background
               COMMAND civalid
               OPTIONS --gamma-background --expand16)
  ci_add_test(NAME civalid-gamma-expand16-transform
               COMMAND civalid
               OPTIONS --gamma-transform --expand16)
  ci_add_test(NAME civalid-gamma-sbit
               COMMAND civalid
               OPTIONS --gamma-sbit)
  ci_add_test(NAME civalid-gamma-threshold
               COMMAND civalid
               OPTIONS --gamma-threshold)
  ci_add_test(NAME civalid-gamma-transform
               COMMAND civalid
               OPTIONS --gamma-transform)
  ci_add_test(NAME civalid-progressive-interlace-standard
               COMMAND civalid
               OPTIONS --standard --progressive-read --interlace)
  ci_add_test(NAME civalid-progressive-size
               COMMAND civalid
               OPTIONS --size --progressive-read)
  ci_add_test(NAME civalid-progressive-standard
               COMMAND civalid
               OPTIONS --standard --progressive-read)
  ci_add_test(NAME civalid-standard
               COMMAND civalid
               OPTIONS --standard)
  ci_add_test(NAME civalid-transform
               COMMAND civalid
               OPTIONS --transform)

  add_executable(cistest ${cistest_sources})
  target_link_libraries(cistest
                        PRIVATE ci_shared)

  foreach(gamma_type 1.8 linear none sRGB)
    foreach(alpha_type none alpha)
      set(CISTEST_FILES)
      foreach(test_ci ${TEST_CIS})
        string(REGEX MATCH "-linear[-.]" TEST_CI_LINEAR "${test_ci}")
        string(REGEX MATCH "-sRGB[-.]" TEST_CI_SRGB "${test_ci}")
        string(REGEX MATCH "-1.8[-.]" TEST_CI_G18 "${test_ci}")
        string(REGEX MATCH "-alpha-" TEST_CI_ALPHA "${test_ci}")

        set(TEST_CI_VALID TRUE)

        if(TEST_CI_ALPHA)
          if(NOT alpha_type STREQUAL "alpha")
            set(TEST_CI_VALID FALSE)
          endif()
        else()
          if(alpha_type STREQUAL "alpha")
            set(TEST_CI_VALID FALSE)
          endif()
        endif()

        if(TEST_CI_LINEAR)
          if(NOT gamma_type STREQUAL "linear")
            set(TEST_CI_VALID FALSE)
          endif()
        elseif(TEST_CI_SRGB)
          if(NOT gamma_type STREQUAL "sRGB")
            set(TEST_CI_VALID FALSE)
          endif()
        elseif(TEST_CI_G18)
          if(NOT gamma_type STREQUAL "1.8")
            set(TEST_CI_VALID FALSE)
          endif()
        else()
          if(NOT gamma_type STREQUAL "none")
            set(TEST_CI_VALID FALSE)
          endif()
        endif()

        if(TEST_CI_VALID)
          list(APPEND CISTEST_FILES "${test_ci}")
        endif()
      endforeach()
      # Should already be sorted, but sort anyway to be certain.
      list(SORT CISTEST_FILES)
      ci_add_test(NAME cistest-${gamma_type}-${alpha_type}
                   COMMAND cistest
                   OPTIONS --tmpfile "${gamma_type}-${alpha_type}-" --log
                   FILES ${CISTEST_FILES})
    endforeach()
  endforeach()

  add_executable(ciunknown ${ciunknown_sources})
  target_link_libraries(ciunknown
                        PRIVATE ci_shared)

  ci_add_test(NAME ciunknown-discard
               COMMAND ciunknown
               OPTIONS --strict default=discard
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-IDAT
               COMMAND ciunknown
               OPTIONS --strict default=discard IDAT=save
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-if-safe
               COMMAND ciunknown
               OPTIONS --strict default=if-safe
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-sAPI
               COMMAND ciunknown
               OPTIONS --strict
                       bKGD=save cHRM=save gAMA=save all=discard iCCP=save sBIT=save sRGB=save
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-save
               COMMAND ciunknown
               OPTIONS --strict default=save
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-sTER
               COMMAND ciunknown
               OPTIONS --strict sTER=if-safe
               FILES "${CITEST_CI}")
  ci_add_test(NAME ciunknown-vpAg
               COMMAND ciunknown
               OPTIONS --strict vpAg=if-safe
               FILES "${CITEST_CI}")

  add_executable(ciimage ${ciimage_sources})
  target_link_libraries(ciimage
                        PRIVATE ci_shared)

  ci_add_test(NAME ciimage-quick
               COMMAND ciimage
               OPTIONS --list-combos --log
               FILES ${CISUITE_CIS})
  ci_add_test(NAME ciimage-full
               COMMAND ciimage
               OPTIONS --exhaustive --list-combos --log
               FILES ${CISUITE_CIS})
endif()

if(CI_SHARED AND CI_TOOLS)
  add_executable(cifix ${cifix_sources})
  target_link_libraries(cifix
                        PRIVATE ci_shared)
  set(CI_BIN_TARGETS cifix)

  add_executable(ci-fix-itxt ${ci_fix_itxt_sources})
  target_link_libraries(ci-fix-itxt
                        PRIVATE ZLIB::ZLIB ${M_LIBRARY})
  list(APPEND CI_BIN_TARGETS ci-fix-itxt)
endif()

# Create a symlink that points to a target file (if symlinking is possible),
# or make a copy of the target file (if symlinking is not possible):
# create_symlink(<destfile> [FILE <file> | TARGET <target>])
function(create_symlink DEST_FILE)
  # TODO:
  # Replace this implementation with CMake's built-in create_symlink function,
  # which has been fully functional on all platforms, including Windows, since
  # CMake version 3.13.
  cmake_parse_arguments(_SYM "" "FILE;TARGET" "" ${ARGN})
  if(NOT _SYM_FILE AND NOT _SYM_TARGET)
    message(FATAL_ERROR "create_symlink: Missing arguments: FILE or TARGET")
  endif()
  if(_SYM_FILE AND _SYM_TARGET)
    message(FATAL_ERROR "create_symlink: Mutually-exlusive arguments:"
                        "FILE (${_SYM_FILE}) and TARGET (${_SYM_TARGET})")
  endif()

  if(_SYM_FILE)
    # If we don't need to symlink something that's coming from a build target,
    # we can go ahead and symlink/copy at configure time.
    if(CMAKE_HOST_WIN32 AND NOT CYGWIN)
      execute_process(COMMAND "${CMAKE_COMMAND}"
                              -E copy_if_different
                              "${_SYM_FILE}"
                              "${DEST_FILE}"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    else()
      execute_process(COMMAND "${CMAKE_COMMAND}"
                              -E create_symlink
                              "${_SYM_FILE}"
                              "${DEST_FILE}"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
  endif()

  if(_SYM_TARGET)
    # We need to use generator expressions, which can be a bit tricky.
    # For simplicity, make the symlink a POST_BUILD step, and use the TARGET
    # signature of add_custom_command.
    if(CMAKE_HOST_WIN32 AND NOT CYGWIN)
      add_custom_command(TARGET ${_SYM_TARGET}
                         POST_BUILD
                         COMMAND "${CMAKE_COMMAND}"
                                 -E copy_if_different
                                 "$<TARGET_LINKER_FILE_DIR:${_SYM_TARGET}>/$<TARGET_LINKER_FILE_NAME:${_SYM_TARGET}>"
                                 "$<TARGET_LINKER_FILE_DIR:${_SYM_TARGET}>/${DEST_FILE}")
    else()
      add_custom_command(TARGET ${_SYM_TARGET}
                         POST_BUILD
                         COMMAND "${CMAKE_COMMAND}"
                                 -E create_symlink
                                 "$<TARGET_LINKER_FILE_NAME:${_SYM_TARGET}>"
                                 "$<TARGET_LINKER_FILE_DIR:${_SYM_TARGET}>/${DEST_FILE}")
    endif()
  endif()
endfunction()

# Create source generation scripts.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/genchk.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/scripts/cmake/genchk.cmake"
               @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/genout.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/scripts/cmake/genout.cmake"
               @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/gensrc.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/scripts/cmake/gensrc.cmake"
               @ONLY)

# libci is a library so default to 'lib'
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif()

# Create pkgconfig files.
# We use the same files like ./configure, so we have to set its vars.
# Only do this on Windows for Cygwin - the files don't make much sense
# outside of a UNIX look-alike.
if(NOT WIN32 OR CYGWIN OR MINGW)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
  set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  set(LIBS "-lz -lm")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libci.pc.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/libci${CILIB_ABI_VERSION}.pc"
                 @ONLY)
  create_symlink(libci.pc FILE libci${CILIB_ABI_VERSION}.pc)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libci-config.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/libci${CILIB_ABI_VERSION}-config"
                 @ONLY)
  create_symlink(libci-config FILE libci${CILIB_ABI_VERSION}-config)
endif()

# Install.
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  install(TARGETS ${CI_LIBRARY_TARGETS}
          EXPORT libci
          RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
          ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
          FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}")

  if(CI_SHARED)
    # Create a symlink for libci.dll.a => libci16.dll.a on Cygwin
    if(NOT WIN32 OR CYGWIN OR MINGW)
      create_symlink(libci${CMAKE_SHARED_LIBRARY_SUFFIX} TARGET ci_shared)
      install(FILES "$<TARGET_LINKER_FILE_DIR:ci_shared>/libci${CMAKE_SHARED_LIBRARY_SUFFIX}"
              DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()

  if(CI_STATIC)
    if(NOT WIN32 OR CYGWIN OR MINGW)
      create_symlink(libci${CMAKE_STATIC_LIBRARY_SUFFIX} TARGET ci_static)
      install(FILES "$<TARGET_LINKER_FILE_DIR:ci_static>/libci${CMAKE_STATIC_LIBRARY_SUFFIX}"
              DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  install(FILES ${libci_public_hdrs}
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(FILES ${libci_public_hdrs}
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libci${CILIB_ABI_VERSION}")
endif()
if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL)
  if(NOT WIN32 OR CYGWIN OR MINGW)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libci-config"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libci${CILIB_ABI_VERSION}-config"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif()
endif()

if(NOT SKIP_INSTALL_PROGRAMS AND NOT SKIP_INSTALL_ALL)
  install(TARGETS ${CI_BIN_TARGETS}
          RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
  # Install the man pages.
  install(FILES libci.3 libcipf.3
          DESTINATION "${CMAKE_INSTALL_MANDIR}/man3")
  install(FILES ci.5
          DESTINATION "${CMAKE_INSTALL_MANDIR}/man5")
  # Install the pkg-config files.
  if(NOT CMAKE_HOST_WIN32 OR CYGWIN OR MINGW)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libci.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libci-config"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libci${CILIB_ABI_VERSION}.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/libci${CILIB_ABI_VERSION}-config"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif()
endif()

# Create an export file that CMake users can include() to import our targets.
if(NOT SKIP_INSTALL_EXPORT AND NOT SKIP_INSTALL_ALL)
  install(EXPORT libci
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/libci"
          FILE libci${CILIB_ABI_VERSION}.cmake)
endif()

# Create a CMake Config File that can be used via find_package(CI CONFIG)
if(NOT SKIP_INSTALL_CONFIG_FILE AND NOT SKIP_INSTALL_ALL)
  install(TARGETS ${CI_LIBRARY_TARGETS}
          EXPORT CITargets
          RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
          ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
          FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}")

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(CIConfigVersion.cmake
                                   VERSION ${CILIB_VERSION}
                                   COMPATIBILITY SameMinorVersion)

  install(EXPORT CITargets
          FILE CITargets.cmake
          NAMESPACE CI::
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CI")

  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/CIConfig.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/CIConfigVersion.cmake"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CI")
endif()

# TODO: Create MSVC import lib for MinGW-compiled shared lib.
# pexports libci.dll > libci.def
# lib /def:libci.def /machine:x86

add_fuzz_test(fuzz fuzz.cpp)
target_link_libraries(fuzz PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,ci_static>")

add_subdirectory(cifuzz-spark)

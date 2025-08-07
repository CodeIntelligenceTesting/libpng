#!/usr/bin/env bash
set -o errexit -o pipefail -o posix

# Copyright (c) 2019-2025 Cosmin Truta.
#
# Use, modification and distribution are subject to the MIT License.
# Please see the accompanying file LICENSE_MIT.txt
#
# SPDX-License-Identifier: MIT

# shellcheck source=ci/lib/ci.lib.sh
source "$(dirname "$0")/lib/ci.lib.sh"
cd "$CI_TOPLEVEL_DIR"

# Declare the global environments collected from various sources.
declare CI_ENV_LIBCI_VER        # collected from ci.h
declare CI_ENV_AUTOCONF_VER      # collected from configure.ac
declare CI_ENV_CMAKE_VER         # collected from CMakeLists.txt
declare CI_ENV_LIBCICONFIG_VER  # collected from scripts/libci-config-head.in

function ci_run_shellify {
    local my_script my_result
    my_script="$CI_SCRIPT_DIR/libexec/ci_shellify_${1#--}.sh"
    shift 1
    [[ -f $my_script ]] || {
        ci_err_internal "missing script: '$my_script'"
    }
    ci_info "shellifying:" "$@"
    "$BASH" "$my_script" "$@"
    echo "$my_result" | "$BASH" --posix || ci_err "bad shellify output"
    echo "$my_result"
}

function ci_init_version_verification {
    ci_info "## START OF VERIFICATION ##"
    CI_ENV_LIBCI_VER="$(ci_run_shellify --c ci.h)"
    echo "$CI_ENV_LIBCI_VER"
    CI_ENV_AUTOCONF_VER="$(ci_run_shellify --autoconf configure.ac)"
    echo "$CI_ENV_AUTOCONF_VER"
    CI_ENV_CMAKE_VER="$(ci_run_shellify --cmake CMakeLists.txt)"
    echo "$CI_ENV_CMAKE_VER"
    CI_ENV_LIBCICONFIG_VER="$(ci_run_shellify --shell scripts/libci-config-head.in)"
    echo "$CI_ENV_LIBCICONFIG_VER"
}

# shellcheck disable=SC2154
function ci_do_version_verification {
    local my_expect
    ci_info "## VERIFYING: version definitions in 'ci.h' ##"
    eval "$CI_ENV_LIBCI_VER"
    my_expect="${CI_LIBCI_VER_MAJOR}.${CI_LIBCI_VER_MINOR}.${CI_LIBCI_VER_RELEASE}"
    if [[ "$CI_LIBCI_VER_STRING" == "$my_expect"* ]]
    then
        ci_info "matched: \$CI_LIBCI_VER_STRING == $my_expect*"
    else
        ci_err "mismatched: \$CI_LIBCI_VER_STRING != $my_expect*"
    fi
    my_expect=$((CI_LIBCI_VER_MAJOR*10000 + CI_LIBCI_VER_MINOR*100 + CI_LIBCI_VER_RELEASE))
    if [[ "$CI_LIBCI_VER" == "$my_expect" ]]
    then
        ci_info "matched: \$CI_LIBCI_VER == $my_expect"
    else
        ci_err "mismatched: \$CI_LIBCI_VER != $my_expect"
    fi
    my_expect=$((CI_LIBCI_VER_MAJOR*10 + CI_LIBCI_VER_MINOR))
    if [[ "$CI_LIBCI_VER_SHAREDLIB" == "$my_expect" ]]
    then
        ci_info "matched: \$CI_LIBCI_VER_SHAREDLIB == $my_expect"
    else
        ci_err "mismatched: \$CI_LIBCI_VER_SHAREDLIB != $my_expect"
    fi
    if [[ "$CI_LIBCI_VER_SONUM" == "$my_expect" ]]
    then
        ci_info "matched: \$CI_LIBCI_VER_SONUM == $my_expect"
    else
        ci_err "mismatched: \$CI_LIBCI_VER_SONUM != $my_expect"
    fi
    if [[ "$CI_LIBCI_VER_DLLNUM" == "$my_expect" ]]
    then
        ci_info "matched: \$CI_LIBCI_VER_DLLNUM == $my_expect"
    else
        ci_err "mismatched: \$CI_LIBCI_VER_DLLNUM != $my_expect"
    fi
    if [[ "$CI_LIBCI_VER_BUILD" == [01] ]]
    then
        ci_info "matched: \$CI_LIBCI_VER_BUILD == [01]"
    else
        ci_err "mismatched: \$CI_LIBCI_VER_BUILD != [01]"
    fi
    ci_info "## VERIFYING: build definitions in 'ci.h' ##"
    my_expect="${CI_LIBCI_VER_MAJOR}.${CI_LIBCI_VER_MINOR}.${CI_LIBCI_VER_RELEASE}"
    if [[ "$CI_LIBCI_VER_STRING" == "$my_expect" ]]
    then
        if [[ $CI_LIBCI_VER_BUILD -eq 0 ]]
        then
            ci_info "matched: \$CI_LIBCI_VER_BUILD -eq 0"
        else
            ci_err "mismatched: \$CI_LIBCI_VER_BUILD -ne 0"
        fi
        if [[ $CI_LIBCI_BUILD_BASE_TYPE -eq $CI_LIBCI_BUILD_STABLE ]]
        then
            ci_info "matched: \$CI_LIBCI_BUILD_BASE_TYPE -eq \$CI_LIBCI_BUILD_STABLE"
        else
            ci_err "mismatched: \$CI_LIBCI_BUILD_BASE_TYPE -ne \$CI_LIBCI_BUILD_STABLE"
        fi
    elif [[ "$CI_LIBCI_VER_STRING" == "$my_expect".git ]]
    then
        if [[ $CI_LIBCI_VER_BUILD -ne 0 ]]
        then
            ci_info "matched: \$CI_LIBCI_VER_BUILD -ne 0"
        else
            ci_err "mismatched: \$CI_LIBCI_VER_BUILD -eq 0"
        fi
        if [[ $CI_LIBCI_BUILD_BASE_TYPE -ne $CI_LIBCI_BUILD_STABLE ]]
        then
            ci_info "matched: \$CI_LIBCI_BUILD_BASE_TYPE -ne \$CI_LIBCI_BUILD_STABLE"
        else
            ci_err "mismatched: \$CI_LIBCI_BUILD_BASE_TYPE -eq \$CI_LIBCI_BUILD_STABLE"
        fi
    else
        ci_err "unexpected: \$CI_LIBCI_VER_STRING == '$CI_LIBCI_VER_STRING'"
    fi
    ci_info "## VERIFYING: type definitions in 'ci.h' ##"
    my_expect="$(echo "ci_libci_version_${CI_LIBCI_VER_STRING}" | tr . _)"
    ci_spawn grep -w -e "$my_expect" ci.h
    ci_info "## VERIFYING: version definitions in 'configure.ac' ##"
    eval "$CI_ENV_AUTOCONF_VER"
    if [[ "$CILIB_VERSION" == "$CI_LIBCI_VER_STRING" ]]
    then
        ci_info "matched: \$CILIB_VERSION == \$CI_LIBCI_VER_STRING"
    else
        ci_err "mismatched: \$CILIB_VERSION != \$CI_LIBCI_VER_STRING"
    fi
    ci_info "## VERIFYING: version definitions in 'CMakeLists.txt' ##"
    eval "$CI_ENV_CMAKE_VER"
    if [[ "$CILIB_VERSION" == "$CI_LIBCI_VER_STRING" && "$CILIB_SUBREVISION" == 0 ]]
    then
        ci_info "matched: \$CILIB_VERSION == \$CI_LIBCI_VER_STRING"
        ci_info "matched: \$CILIB_SUBREVISION == 0"
    elif [[ "$CILIB_VERSION.$CILIB_SUBREVISION" == "$CI_LIBCI_VER_STRING" ]]
    then
        ci_info "matched: \$CILIB_VERSION.\$CILIB_SUBREVISION == \$CI_LIBCI_VER_STRING"
    else
        ci_err "mismatched: \$CILIB_VERSION != \$CI_LIBCI_VER_STRING"
    fi
    ci_info "## VERIFYING: version definitions in 'scripts/libci-config-head.in' ##"
    eval "$CI_ENV_LIBCICONFIG_VER"
    if [[ "$version" == "$CI_LIBCI_VER_STRING" ]]
    then
        ci_info "matched: \$version == \$CI_LIBCI_VER_STRING"
    else
        ci_err "mismatched: \$version != \$CI_LIBCI_VER_STRING"
    fi
}

function ci_finish_version_verification {
    ci_info "## END OF VERIFICATION ##"
    # Relying on "set -o errexit" to not reach here in case of error.
    ci_info "## SUCCESS ##"
}

function ci_verify_version {
    ci_init_version_verification
    ci_do_version_verification
    ci_finish_version_verification
}

function usage {
    echo "usage: $CI_SCRIPT_NAME [<options>]"
    echo "options: -?|-h|--help"
    exit "${@:-0}"
}

function main {
    local opt
    while getopts ":" opt
    do
        # This ain't a while-loop. It only pretends to be.
        [[ $1 == -[?h]* || $1 == --help || $1 == --help=* ]] && usage 0
        ci_err "unknown option: '$1'"
    done
    shift $((OPTIND - 1))
    [[ $# -eq 0 ]] || {
        echo >&2 "error: unexpected argument: '$1'"
        usage 2
    }
    # And... go!
    ci_verify_version
}

main "$@"

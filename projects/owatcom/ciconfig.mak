# This is an OpenWatcom make file which builds cilibconf.h - the libci
# configuration header.  You can ignore this file if you don't need to
# configure libci; a default configuration will be built.
#
# For more information build libci.wpj under the IDE and then read the
# generated files:
#
#    config.inf: Basic configuration information for a standard build.
#    ciconfig.dfa: Advanced configuration for non-standard libci builds.
#
DELETE=rm -f
ECHO=echo
COPY=copy
#
# If your configuration needs to test compiler flags when building
# cilibconf.h you may need to override the following on the wmake command
# line:
CFLAGS=
CC=wcl386
CPP=$(CC) -pw0
#
# Read awk from the environment if set, else it can be set on the command
# line (the default approach is to set the %awk% environment variable in the
# IDE libci.wpj 'before' rule - this setting is local.)
!ifdef %awk
AWK=$(%awk)
!endif
#
# cilibconf.h must exist in the source directory, this is the final rule
# which copies the local built version (and this is the default target for
# this makefile.)
..\..\cilibconf.h: cilibconf.h
 $(COPY) cilibconf.h $@

!ifdef AWK
# CPPFLAGS should contain the options to control the result,
# but DEFS and CFLAGS are also supported here, override
# as appropriate
DFNFLAGS = $(DEFS) $(CPPFLAGS) $(CFLAGS)

cilibconf.h: cilibconf.dfn
 $(DELETE) $@ dfn.c dfn1.out dfn2.out
 $(ECHO) $#include "cilibconf.dfn" >dfn.c
 $(CPP) $(DFNFLAGS) dfn.c >dfn1.out
 $(AWK) -f << dfn1.out >dfn2.out
/^.*CI_DEFN_MAGIC-.*-CI_DEFN_END.*$$/{
 sub(/^.*CI_DEFN_MAGIC-/, "")
 sub(/ *-CI_DEFN_END.*$$/, "")
 gsub(/ *@@@ */, "")
 print
}
<<
 $(COPY) dfn2.out $@
 @type << >ciconfig.inf
This is a locally configurable build of libci.lib; for configuration
instructions consult and edit projects/openwatcom/ciconfig.dfa
<<
 $(DELETE) dfn.c dfn1.out dfn2.out

cilibconf.dfn: ..\..\scripts\cilibconf.dfa ..\..\scripts\options.awk ciconfig.dfa ..\..\ciconf.h
 $(DELETE) $@ dfn1.out dfn2.out
 $(AWK) -f ..\..\scripts\options.awk out=dfn1.out version=search ..\..\ciconf.h ..\..\scripts\cilibconf.dfa ciconfig.dfa $(DFA_XTRA) 1>&2
 $(AWK) -f ..\..\scripts\options.awk out=dfn2.out dfn1.out 1>&2
 $(COPY) dfn2.out $@
 $(DELETE) dfn1.out dfn2.out

!else
# The following lines are used to copy scripts\cilibconf.h.prebuilt and make
# the required change to the calling convention.
#
# By default libci is built to use the __cdecl calling convention on
# Windows.  This gives compatibility with MSVC and GCC.  Unfortunately it
# does not work with OpenWatcom because OpenWatcom implements longjmp using
# the __watcall convention (compared with both MSVC and GCC which use __cdecl
# for library functions.)
#
# Thus the default must be changed to build on OpenWatcom and, once changed,
# the result will not be compatible with applications built using other
# compilers (in fact attempts to build will fail at compile time.)
#
cilibconf.h: ..\..\scripts\cilibconf.h.prebuilt .existsonly
 @$(ECHO) .
 @$(ECHO) .
 @$(ECHO) $$(AWK) NOT AVAILABLE: COPYING scripts\cilibconf.h.prebuilt
 @$(ECHO) .
 @$(ECHO) .
 vi -q -k ":1,$$s/CI_API_RULE 0$$/CI_API_RULE 2/\n:w! $@\n:q!\n" ..\..\scripts\cilibconf.h.prebuilt
 @$(ECHO) .
 @$(ECHO) .
 @$(ECHO) YOU HAVE A DEFAULT CONFIGURATION BECAUSE YOU DO NOT HAVE AWK!
 @$(ECHO) .
 @$(ECHO) .
 @type << >ciconfig.inf
This is the default configuration of libci.lib, if you wish to
change the configuration please consult the instructions in
projects/owatcom/ciconfig.dfa.
<<

!endif

# Make the default files
defaults: .symbolic
 @$(COPY) << config.inf
$# The libci project is incompletely configured.  To complete configuration
$# please complete the following steps:
$#
$#   1) Edit the 'before' rule of libci.wpj (from the IDE) to define the
$#      locations of the zlib include file zlib.h and the built zlib library,
$#      zlib.lib.
$#
$#   2) If you want to change libci to a non-standard configuration also
$#      change the definition of 'awk' in the before rule to the name of your
$#      awk command.  For more instructions on configuration read
$#      ciconfig.dfa.
$#
$#   3) Delete this file (config.inf).
<<
 @$(COPY) << ciconfig.dfa
$# ciconfig.dfa: this file contains configuration options for libci.
$# If empty the standard configuration will be built.  For this file to be
$# used a working version of the program 'awk' is required and the program
$# must be identified in the 'before' rule of the project.
$#
$# If you don't already have 'awk', or the version of awk you have seems not
$# to work, download Brian Kernighan's awk (Brian Kernighan is the author of
$# awk.)  You can find source code and a built executable (called awk95.exe)
$# here:
$#
$#     https://www.cs.princeton.edu/~bwk/btl.mirror/
$#
$# The executable works just fine.
$#
$# If build issues errors after a change to ciconfig.dfa you have entered
$# inconsistent feature requests, or even malformed requests, in
$# ciconfig.dfa.  The error messages from awk should be comprehensible, but
$# if not simply go back to the start (nothing but comments in this file) and
$# enter configuration lines one by one until one produces an error.  (Or, of
$# course, do the standard binary chop.)
$#
$# You need to rebuild everything after a change to cilibconf.dfa - i.e. you
$# must do Actions/Mark All Targets for Remake.  This is because the compiler
$# generated dependency information (as of OpenWatcom 1.9) does not record the
$# dependency on cilibconf.h correctly.
$#
$# If awk isn't set then this file is bypassed.  If you just want the standard
$# configuration it is automatically produced from the distributed version
$# (scripts\cilibconf.h.prebuilt) by editing CI_API_RULE to 2 (to force use
$# of the OpenWatcom library calling convention.)
$#
<<

clean:: .symbolic
 $(DELETE) ..\..\cilibconf.h cilibconf.* dfn.c *.out ciconfig.inf
 $(DELETE) *.obj *.mbr *.sym *.err *.pch libci.mk
 $(DELETE) libci.lib libci.lbr libci.lb1 libci.cbr libci.mk1
 $(DELETE) citest.exe citest.map citest.lk1 citest.mk1
 $(DELETE) civalid.exe civalid.map civalid.lk1 civalid.mk1

distclean:: clean .symbolic
 $(DELETE) zlib.inf awk.inf config.inf ciconfig.dfa

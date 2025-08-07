# Makefile.am, the source file for Makefile.in (and hence Makefile), is
#
# Copyright (c) 2018-2025 Cosmin Truta
# Copyright (c) 2004-2016 Glenn Randers-Pehrson
#
# This code is released under the libci license.
# For conditions of distribution and use, see the disclaimer
# and license in ci.h

CILIB_BASENAME= libci@CILIB_MAJOR@@CILIB_MINOR@

ACLOCAL_AMFLAGS = -I scripts/autoconf

# test programs - run on make check, make distcheck
if ENABLE_TESTS
check_PROGRAMS= citest ciunknown cistest civalid ciimage cicp
if HAVE_CLOCK_GETTIME
check_PROGRAMS += timeci
endif
else
check_PROGRAMS=
endif

# Utilities - installed
if ENABLE_TOOLS
bin_PROGRAMS= cifix ci-fix-itxt
else
bin_PROGRAMS=
endif

# This ensures that cilibconf.h gets built at the start of 'make all' or
# 'make check', but it does not add dependencies to the individual programs,
# this is done below.
#
# IMPORTANT: always add the object modules of new programs to the list below
# because otherwise the sequence 'configure; make new-program' will *sometimes*
# result in the installed (system) cilibconf.h being used and the result is
# always wrong and always very confusing.
BUILT_SOURCES = cilibconf.h

if ENABLE_TESTS
citest_SOURCES = citest.c
citest_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

civalid_SOURCES = contrib/libtests/civalid.c
civalid_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

cistest_SOURCES = contrib/libtests/cistest.c
cistest_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

ciunknown_SOURCES = contrib/libtests/ciunknown.c
ciunknown_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

ciimage_SOURCES = contrib/libtests/ciimage.c
ciimage_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

timeci_SOURCES = contrib/libtests/timeci.c
timeci_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

cicp_SOURCES = contrib/tools/cicp.c
cicp_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la
endif

if ENABLE_TOOLS
cifix_SOURCES = contrib/tools/cifix.c
cifix_LDADD = libci@CILIB_MAJOR@@CILIB_MINOR@.la

ci_fix_itxt_SOURCES = contrib/tools/ci-fix-itxt.c
endif

# Generally these are single line shell scripts to run a test with a particular
# set of parameters:
if ENABLE_TESTS
TESTS =\
   tests/citest-all\
   tests/civalid-gamma-16-to-8 tests/civalid-gamma-alpha-mode\
   tests/civalid-gamma-background tests/civalid-gamma-expand16-alpha-mode\
   tests/civalid-gamma-expand16-background\
   tests/civalid-gamma-expand16-transform tests/civalid-gamma-sbit\
   tests/civalid-gamma-threshold tests/civalid-gamma-transform\
   tests/civalid-progressive-size\
   tests/civalid-progressive-interlace-standard\
   tests/civalid-transform\
   tests/civalid-progressive-standard tests/civalid-standard\
   tests/cistest-1.8 tests/cistest-1.8-alpha tests/cistest-linear\
   tests/cistest-linear-alpha tests/cistest-none tests/cistest-none-alpha\
   tests/cistest-sRGB tests/cistest-sRGB-alpha tests/ciunknown-IDAT\
   tests/ciunknown-discard tests/ciunknown-if-safe tests/ciunknown-sAPI\
   tests/ciunknown-sTER tests/ciunknown-save tests/ciunknown-vpAg\
   tests/ciimage-quick tests/ciimage-full
endif

# man pages
dist_man_MANS= libci.3 libcipf.3 ci.5

# generate the -config scripts if required
binconfigs= libci@CILIB_MAJOR@@CILIB_MINOR@-config
EXTRA_SCRIPTS= libci-config libci@CILIB_MAJOR@@CILIB_MINOR@-config
bin_SCRIPTS= @binconfigs@

# rules to build libci, only build the old library on request
lib_LTLIBRARIES=libci@CILIB_MAJOR@@CILIB_MINOR@.la
# EXTRA_LTLIBRARIES= libci.la
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES = ci.c cierror.c\
	ciget.c cimem.c cipread.c ciread.c cirio.c cirtran.c cirutil.c\
	ciset.c citrans.c ciwio.c ciwrite.c ciwtran.c ciwutil.c\
	ci.h ciconf.h cidebug.h ciinfo.h cipriv.h cistruct.h ciusr.dfa

if CI_ARM_NEON
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += arm/arm_init.c\
	arm/filter_neon_intrinsics.c \
	arm/palette_neon_intrinsics.c
endif

if CI_MIPS_MSA
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += mips/mips_init.c\
	mips/filter_msa_intrinsics.c
endif

if CI_MIPS_MMI
if !CI_MIPS_MSA
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += mips/mips_init.c
endif
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += mips/filter_mmi_inline_assembly.c
endif

if CI_INTEL_SSE
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += intel/intel_init.c\
	intel/filter_sse2_intrinsics.c
endif

if CI_POWERPC_VSX
libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES += powerpc/powerpc_init.c\
        powerpc/filter_vsx_intrinsics.c
endif

if CI_RISCV_RVV
noinst_LTLIBRARIES= libci@CILIB_MAJOR@@CILIB_MINOR@rvv.la
libci@CILIB_MAJOR@@CILIB_MINOR@rvv_la_SOURCES = riscv/riscv_init.c\
        riscv/filter_rvv_intrinsics.c
libci@CILIB_MAJOR@@CILIB_MINOR@rvv_la_CFLAGS = -march=rv64gv
libci@CILIB_MAJOR@@CILIB_MINOR@_la_LIBADD = libci@CILIB_MAJOR@@CILIB_MINOR@rvv.la
endif

if CI_LOONGARCH_LSX
noinst_LTLIBRARIES= libci@CILIB_MAJOR@@CILIB_MINOR@lsx.la
libci@CILIB_MAJOR@@CILIB_MINOR@lsx_la_SOURCES = loongarch/loongarch_lsx_init.c\
	loongarch/filter_lsx_intrinsics.c
libci@CILIB_MAJOR@@CILIB_MINOR@lsx_la_CFLAGS = -mlsx
libci@CILIB_MAJOR@@CILIB_MINOR@_la_LIBADD = libci@CILIB_MAJOR@@CILIB_MINOR@lsx.la
endif

nodist_libci@CILIB_MAJOR@@CILIB_MINOR@_la_SOURCES = cilibconf.h

libci@CILIB_MAJOR@@CILIB_MINOR@_la_LDFLAGS = -no-undefined -export-dynamic \
	-version-number @CILIB_MAJOR@@CILIB_MINOR@:@CILIB_RELEASE@:0

if HAVE_LD_VERSION_SCRIPT
#   Versioned symbols and restricted exports
if HAVE_SOLARIS_LD
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_LDFLAGS += -Wl,-M -Wl,libci.vers
else
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_LDFLAGS += -Wl,--version-script=libci.vers
endif

  libci@CILIB_MAJOR@@CILIB_MINOR@_la_DEPENDENCIES = libci.vers
else
#   Only restricted exports when possible
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_LDFLAGS += -export-symbols libci.sym
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_DEPENDENCIES = libci.sym
endif

if CI_RISCV_RVV
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_DEPENDENCIES += libci@CILIB_MAJOR@@CILIB_MINOR@rvv.la
endif

if CI_LOONGARCH_LSX
  libci@CILIB_MAJOR@@CILIB_MINOR@_la_DEPENDENCIES += libci@CILIB_MAJOR@@CILIB_MINOR@lsx.la
endif

#distribute headers in /usr/include/libci/*
pkgincludedir= $(includedir)/$(CILIB_BASENAME)
pkginclude_HEADERS= ci.h ciconf.h
nodist_pkginclude_HEADERS= cilibconf.h

# pkg-config stuff, note that libci.pc is always required in order
# to get the correct library
pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libci@CILIB_MAJOR@@CILIB_MINOR@.pc

# Extra source distribution files, '${srcdir}' is used below to stop build files
# from those directories being included.  This only works if the configure is
# not done in the source directory!
EXTRA_DIST= \
	ANNOUNCE AUTHORS CHANGES INSTALL LICENSE README TODO TRADEMARK \
	citest.ci cibar.ci cinow.ci cibar.jpg autogen.sh \
	${srcdir}/ci ${srcdir}/contrib ${srcdir}/projects ${srcdir}/scripts \
	$(TESTS) $(XFAIL_TESTS) tests/cistest \
	CMakeLists.txt example.c libci-manual.txt

SCRIPT_CLEANFILES=scripts/*.out scripts/*.chk

CLEANFILES= *.tf? ciout.ci libci@CILIB_MAJOR@@CILIB_MINOR@.pc \
	libci@CILIB_MAJOR@@CILIB_MINOR@-config libci.vers libci.sym \
	check.new cilibconf.h ciprefix.h symbols.new citest-log.txt \
	cilibconf.out cilibconf.c cilibconf.pre cilibconf.dfn \
	$(SCRIPT_CLEANFILES)

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
config.sub configure depcomp install-sh ltmain.sh missing

# CI_COPTS give extra options for the C compiler to be used on all compilation
# steps (unless target_CFLAGS is specified; that will take precedence over
# AM_CFLAGS)
CI_COPTS = @CI_COPTS@
AM_CFLAGS = ${CI_COPTS}

# DFNCPP is normally just CPP - the C preprocessor - but on Solaris and maybe
# other operating systems (NeXT?) the C preprocessor selected by configure
# checks input tokens for validity - effectively it performs part of the ANSI-C
# parsing - and therefore fails with the .df files.  configure.ac has special
# checks for this and sets DFNCPP appropriately.
DFNCPP = @DFNCPP@

SUFFIXES = .chk .out

$(CILIB_BASENAME).pc: libci.pc
	cp libci.pc $@

$(CILIB_BASENAME)-config: libci-config
	cp libci-config $@

scripts/sym.out scripts/vers.out: ci.h ciconf.h cilibconf.h
scripts/prefix.out: ci.h ciconf.h cilibconf.out
scripts/symbols.out: ci.h ciconf.h $(srcdir)/scripts/cilibconf.h.prebuilt
scripts/intprefix.out: cilibconf.h

libci.sym: scripts/sym.out
	rm -f $@
	cp $? $@
libci.vers: scripts/vers.out
	rm -f $@
	cp $? $@

if DO_CI_PREFIX
# Rename functions in scripts/prefix.out with a CI_PREFIX prefix.
# Rename macros in scripts/macro.lst from CI_PREFIXci_ to CI_ (the actual
# implementation of the macro).
cilibconf.h: cilibconf.out scripts/prefix.out scripts/macro.lst
	rm -f $@
	$(AWK) 's==0 && NR>1{print prev}\
	   s==0{prev=$$0}\
	   s==1{print "#define", $$1, "@CI_PREFIX@" $$1}\
	   s==2{print "#define @CI_PREFIX@ci_" $$1, "CI_" $$1}\
	   END{print prev}' s=0 cilibconf.out s=1 scripts/prefix.out\
	   s=2 ${srcdir}/scripts/macro.lst >cilibconf.tf8
	mv cilibconf.tf8 $@

ciprefix.h: scripts/intprefix.out
	rm -f ciprefix.tf1
	$(AWK) '{print "#define", $$1, "@CI_PREFIX@" $$1}' $? >ciprefix.tf1
	mv ciprefix.tf1 $@
else
cilibconf.h: cilibconf.out
	rm -f $@
	cp $? $@

ciprefix.h: # is empty
	:>$@
endif

$(srcdir)/scripts/cilibconf.h.prebuilt:
	@echo "Attempting to build $@" >&2
	@echo "This is a machine generated file, but if you want to make" >&2
	@echo "a new one simply make 'scripts/cilibconf.out', copy that" >&2
	@echo "AND set CI_ZLIB_VERNUM to 0 (you MUST do this)" >&2
	@exit 1

# The following is necessary to ensure that the local cilibconf.h is used, not
# an installed one (this can happen immediately after on a clean system if
# 'make test' is the first thing the user does.)  Only files which include
# one of the ci source files (typically ci.h or cipriv.h) need to be listed
# here:
citest.o: cilibconf.h

contrib/libtests/makeci.o: cilibconf.h
contrib/libtests/cistest.o: cilibconf.h
contrib/libtests/ciunknown.o: cilibconf.h
contrib/libtests/ciimage.o: cilibconf.h
contrib/libtests/civalid.o: cilibconf.h
contrib/libtests/readci.o: cilibconf.h
contrib/libtests/tarith.o: cilibconf.h
contrib/libtests/timeci.o: cilibconf.h

contrib/tools/makesRGB.o: cilibconf.h
contrib/tools/cifix.o: cilibconf.h
contrib/tools/cicp.o: cilibconf.h

# We must use -DCI_NO_USE_READ_MACROS here even when the library may actually
# be built with CI_USE_READ_MACROS; this prevents the read macros from
# interfering with the symbol file format.
SYMBOL_CFLAGS = -DCILIB_LIBNAME='CI@CILIB_MAJOR@@CILIB_MINOR@_0'\
		-DCILIB_VERSION='@CILIB_VERSION@'\
		-DSYMBOL_PREFIX='$(SYMBOL_PREFIX)'\
		-DCI_NO_USE_READ_MACROS -DCI_BUILDING_SYMBOL_TABLE

if DO_CI_PREFIX
SYMBOL_CFLAGS += -DCI_PREFIX='@CI_PREFIX@'
endif

.c.out:
	rm -f $@ $*.tf[12]
	test -d scripts || mkdir scripts || test -d scripts
	$(DFNCPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES)\
	    $(CPPFLAGS) $(SYMBOL_CFLAGS) $< > $*.tf1
	$(AWK) -f "${srcdir}/scripts/dfn.awk" out="$*.tf2" $*.tf1 1>&2
	rm -f $*.tf1
	mv $*.tf2 $@

# The .c file for cilibconf.h is machine generated
cilibconf.c: scripts/cilibconf.dfa scripts/options.awk ciconf.h ciusr.dfa $(DFA_XTRA)
	rm -f $@ $*.tf[45]
	$(AWK) -f ${srcdir}/scripts/options.awk out=$*.tf4 version=search\
	    ${srcdir}/ciconf.h ${srcdir}/scripts/cilibconf.dfa\
	    ${srcdir}/ciusr.dfa $(DFA_XTRA) 1>&2
	$(AWK) -f ${srcdir}/scripts/options.awk out=$*.tf5 $*.tf4 1>&2
	rm $*.tf4
	mv $*.tf5 $@

# Symbol checks (.def and .out files should match)
scripts/symbols.chk: scripts/checksym.awk scripts/symbols.def scripts/symbols.out

.out.chk:
	rm -f $@ $*.new
	$(AWK) -f ${srcdir}/scripts/checksym.awk ${srcdir}/scripts/${*F}.def\
	    of="$*.new" $< >&2
	mv $*.new $@

# used on demand to regenerate the standard header, CPPFLAGS should
# be empty - no non-standard defines
scripts/cilibconf.c: scripts/cilibconf.dfa scripts/options.awk ciconf.h
	rm -f $@ cilibconf.tf[67]
	test -z "$(CPPFLAGS)"
	echo "com @CILIB_VERSION@ STANDARD API DEFINITION" |\
	$(AWK) -f ${srcdir}/scripts/options.awk out=cilibconf.tf6\
	    logunsupported=1 version=search ${srcdir}/ciconf.h -\
	    ${srcdir}/scripts/cilibconf.dfa 1>&2
	$(AWK) -f ${srcdir}/scripts/options.awk out=cilibconf.tf7\
	    cilibconf.tf6 1>&2
	rm cilibconf.tf6
	mv cilibconf.tf7 $@

$(libci@CILIB_MAJOR@@CILIB_MINOR@_la_OBJECTS): ci.h ciconf.h \
	cilibconf.h cipriv.h cidebug.h ciinfo.h cistruct.h ciprefix.h

test: check-am

# Extra checks
check: scripts/symbols.chk

# Don't distribute the generated script files
dist-hook:
	cd '$(top_distdir)'; rm -f $(SCRIPT_CLEANFILES)

# Make links between installed files with release-specific names and the generic
# file names.  If this install rule is run the generic names will be deleted and
# recreated - this has obvious issues for systems with multiple installations.

install-header-links:
	@set -ex; cd '$(DESTDIR)$(includedir)'; for f in $(HEADERS); do \
	   rm -f "$$f"; $(LN_S) "$(CILIB_BASENAME)/$$f" "$$f"; done

uninstall-header-links:
	cd '$(DESTDIR)$(includedir)'; rm -f $(HEADERS)

install-libci-pc:
	@set -ex; cd '$(DESTDIR)$(pkgconfigdir)'; rm -f libci.pc; \
	   $(LN_S) '$(CILIB_BASENAME).pc' libci.pc

uninstall-libci-pc:
	rm -f '$(DESTDIR)$(pkgconfigdir)/libci.pc'

# EXT_LIST is a list of the possibly library directory extensions, this exists
# because we can't find a good way of discovering the file extensions that are
# actually installed on a given system, so instead we check for every extension
# we have seen.

EXT_LIST = a dll.a so so.@CILIB_MAJOR@@CILIB_MINOR@.@CILIB_RELEASE@ la sl dylib

install-library-links:
	@set -x; cd '$(DESTDIR)$(libdir)';\
	for ext in $(EXT_LIST); do\
	   rm -f "libci.$$ext";\
           if test -f "$(CILIB_BASENAME).$$ext"; then\
              $(LN_S) "$(CILIB_BASENAME).$$ext" "libci.$$ext" || exit 1;\
           fi;\
	done

uninstall-library-links:
	@set -x; cd '$(DESTDIR)$(libdir)'; for ext in $(EXT_LIST); do\
	   rm -f "libci.$$ext"; done

install-libci-config:
	@set -ex; cd '$(DESTDIR)$(bindir)'; rm -f libci-config; \
	   $(LN_S) '$(CILIB_BASENAME)-config' libci-config

uninstall-libci-config:
	rm -f '$(DESTDIR)$(bindir)/libci-config'

if DO_INSTALL_LINKS
# If --enable-unversioned-links is specified the header and lib file links
# will be automatically made on a 'make install':

install-data-hook: install-header-links
uninstall-hook: uninstall-header-links
install-exec-hook: install-library-links
uninstall-hook: uninstall-library-links
endif

if DO_INSTALL_LIBCI_PC
# Likewise, --install-pc causes libci.pc to be constructed:

install-data-hook: install-libci-pc
uninstall-hook: uninstall-libci-pc
endif

if DO_INSTALL_LIBCI_CONFIG
# And --install-config:

install-exec-hook: install-libci-config
uninstall-hook: uninstall-libci-config
endif

# The following addition ensures that 'make all' always builds the test programs
# too.  It used to, but some change either in libci or configure stopped this
# working.
all-am: $(check_PROGRAMS)

#!/usr/bin/make -f
# cilibconf.mak - standard make lines for cilibconf.h
#
# These lines are copied from Makefile.am, they illustrate
# how to automate the build of cilibconf.h from scripts/cilibconf.dfa
# given just 'awk', a C preprocessor and standard command line utilities

# Override as appropriate, these definitions can be overridden on
# the make command line (AWK='nawk' for example).
AWK = gawk
AWK = mawk
AWK = nawk
AWK = one-true-awk
AWK = awk      # This fails on SunOS 5.10; use 'nawk'
CPP = $(CC) -E # If this fails on SunOS 5.10, use '/lib/cpp'

MOVE = mv -f
DELETE = rm -f

DFA_XTRA = # Put your configuration file here, see scripts/cilibconf.dfa.  Eg:
# DFA_XTRA = ciusr.dfa

# CPPFLAGS should contain the options to control the result,
# but DEFS and CFLAGS are also supported here, override
# as appropriate
DFNFLAGS = $(DEFS) $(CPPFLAGS) $(CFLAGS)

# srcdir is a de-facto standard for the location of the source
srcdir = .

# The standard cilibconf.h exists as scripts/cilibconf.h.prebuilt,
# copy this if the following doesn't work.
cilibconf.h: cilibconf.dfn
	$(DELETE) $@ cilibconf.c cilibconf.out cilibconf.tmp
	echo '#include "cilibconf.dfn"' >cilibconf.c
	@echo "## If '$(CC) -E' fails, try /lib/cpp (e.g. CPP='/lib/cpp')" >&2
	$(CPP) $(DFNFLAGS) cilibconf.c >cilibconf.out
	$(AWK) -f $(srcdir)/scripts/dfn.awk out=cilibconf.tmp cilibconf.out >&2
	$(MOVE) cilibconf.tmp $@

cilibconf.dfn: $(srcdir)/scripts/cilibconf.dfa $(srcdir)/scripts/options.awk $(srcdir)/ciconf.h $(srcdir)/ciusr.dfa $(DFA_XTRA)
	$(DELETE) $@ cilibconf.pre cilibconf.tmp
	@echo "## Calling $(AWK) from scripts/cilibconf.mak" >&2
	@echo "## If 'awk' fails, try a better awk (e.g. AWK='nawk')" >&2
	$(AWK) -f $(srcdir)/scripts/options.awk out=cilibconf.pre\
	    version=search $(srcdir)/ciconf.h $(srcdir)/scripts/cilibconf.dfa\
	    $(srcdir)/ciusr.dfa $(DFA_XTRA) >&2
	$(AWK) -f $(srcdir)/scripts/options.awk out=cilibconf.tmp cilibconf.pre >&2
	$(MOVE) cilibconf.tmp $@

clean-cilibconf:
	$(DELETE) cilibconf.h cilibconf.c cilibconf.out cilibconf.pre \
	cilibconf.dfn

clean: clean-cilibconf

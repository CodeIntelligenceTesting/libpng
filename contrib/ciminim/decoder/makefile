# Makefile for CiMinus (cim2pnm)
# Linux / Unix

#CC=cc
CC=gcc
LD=$(CC)

# If awk fails try
# make AWK=nawk

# If cpp fails try
# make CPP=/lib/cpp

RM=rm -f
COPY=cp

CPPFLAGS=-I. -DCI_USER_CONFIG -DNO_GZCOMPRESS -DZ_SOLO -DNO_GZIP
CFLAGS=-O1 -Wall

C=.c
O=.o
L=.a
E=

# Where to find the source code:
CISRC =../../..
ZLIBSRC=$(CISRC)/../zlib
PROGSRC=$(CISRC)/contrib/ciminus

# Zlib (minimal inflate requirements - crc32 is used by libci)
# zutil can be eliminated if you provide your own zcalloc and zcfree
ZSRCS  = adler32$(C) crc32$(C) \
	 inffast$(C) inflate$(C) inftrees$(C) \
	 zutil$(C)

# Standard headers
ZH     = zlib.h crc32.h inffast.h inffixed.h \
	 inflate.h inftrees.h zutil.h

# Machine generated headers
ZCONF  = zconf.h

# Headers callers use
ZINC   = zlib.h $(ZCONF)

# Headers the Zlib source uses
ZHDRS  = $(ZH) $(ZCONF)

ZOBJS  = adler32$(O) crc32$(O) \
	 inffast$(O) inflate$(O) inftrees$(O) \
	 zutil$(O)

# libci
CISRCS=ci$(C) cierror$(C) ciget$(C) cimem$(C) \
	ciread$(C) cirio$(C) cirtran$(C) cirutil$(C) \
	ciset$(C) citrans$(C)

# Standard headers
CIH   =ci.h ciconf.h cidebug.h ciinfo.h cipriv.h cistruct.h

# Machine generated headers
CICONF=cilibconf.h

# Headers callers use
CIINC= ci.h ciconf.h ciusr.h $(CICONF)

# Headers the CI library uses
CIHDRS=$(CIH) $(CICONF) ciusr.h

CIOBJS=ci$(O) cierror$(O) ciget$(O) cimem$(O) \
	ciread$(O) cirio$(O) cirtran$(O) cirutil$(O) \
	ciset$(O) citrans$(O)

PROGSRCS= cim2pnm$(C)
PROGHDRS=
PROGDOCS=
PROGOBJS= cim2pnm$(O)

OBJS    = $(PROGOBJS) $(CIOBJS) $(ZOBJS)

# implicit make rules -------------------------------------------------------

# note: dependencies do not work on implicit rule lines
.c$(O):
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

# dependencies

all: cim2pnm$(E)

cim2pnm$(E): $(OBJS)
	$(LD) -o cim2pnm$(E) $(OBJS)

# The DFA_XTRA setting turns all libci options off then
# turns on those required for this minimal build.
# The CPP_FLAGS setting causes ciusr.h to be included in
# both the build of cilibconf.h and, subsequently, when
# building libci itself.
$(CICONF): $(CISRC)/scripts/cilibconf.mak $(ZH)\
	$(CISRC)/scripts/cilibconf.dfa \
	$(CISRC)/scripts/options.awk ciusr.h ciusr.dfa
	$(RM) cilibconf.h cilibconf.dfn
	$(MAKE) -f $(CISRC)/scripts/cilibconf.mak $(MAKEFLAGS)\
	    srcdir=$(CISRC) CPPFLAGS="-DCI_USER_CONFIG -I."\
	    DFA_XTRA="ciusr.dfa" $@

clean:
	$(MAKE) -f $(CISRC)/scripts/cilibconf.mak $(MAKEFLAGS)\
	    srcdir=$(CISRC) clean
	$(RM) cim2pnm$(O)
	$(RM) cim2pnm$(E)
	$(RM) $(OBJS)

# distclean also removes the copied source and headers
distclean: clean
	$(RM) -r scripts # historical reasons
	$(RM) $(CISRCS) $(CIH)
	$(RM) $(ZSRCS) $(ZH) $(ZCONF)
	$(RM) $(PROGSRCS) $(PROGHDRS) $(PROGDOCS)

# Header file dependencies:
$(PROGOBJS): $(PROGHDRS) $(CIINC) $(ZINC)
$(CIOBJS): $(CIHDRS) $(ZINC)
$(ZOBJS): $(ZHDRS)

# Gather the source code from the respective directories
$(CISRCS) $(CIH): $(CISRC)/$@
	$(RM) $@
	$(COPY) $(CISRC)/$@ $@

# No dependency on the ZLIBSRC target so that it only needs
# to be specified once.
$(ZSRCS) $(ZH):
	$(RM) $@
	$(COPY) $(ZLIBSRC)/$@ $@

# The unconfigured zconf.h varies in name according to the
# zlib release
$(ZCONF):
	$(RM) $@
	@for f in zconf.h.in zconf.in.h zconf.h; do\
	    test -r $(ZLIBSRC)/$$f &&\
	    echo $(COPY) $(ZLIBSRC)/$$f $@ &&\
	    $(COPY) $(ZLIBSRC)/$$f $@ && exit 0;\
	done; echo copy: $(ZLIBSRC)/zconf.h not found; exit 1

cim2pnm.c: $(PROGSRC)/ci2pnm.c
	$(RM) $@
	$(COPY) $(PROGSRC)/ci2pnm.c $@

# End of makefile for cim2pnm

# Makefile for CiMinus (pnm2cim)
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

# Zlib
ZSRCS  = adler32$(C) compress$(C) crc32$(C) deflate$(C) \
	 trees$(C) zutil$(C)

# Standard headers
#ZH     = zlib.h crc32.h deflate.h trees.h zutil.h
ZH     = zlib.h crc32.h deflate.h trees.h zutil.h

# Machine generated headers
ZCONF  = zconf.h

# Headers callers use
ZINC   = zlib.h $(ZCONF)

# Headers the Zlib source uses
ZHDRS  = $(ZH) $(ZCONF)

# compress is not required; it is needed to link the zlib
# code because deflate defines an unused API function deflateBound
# which itself calls compressBound from compress.
ZOBJS  = adler32$(O) compress$(O) crc32$(O) deflate$(O) \
	 trees$(O) zutil$(O)

# libci
CISRCS=ci$(C) cierror$(C) ciget$(C) cimem$(C) \
	ciset$(C) citrans$(C) ciwio$(C) ciwrite$(C) \
	ciwtran$(C) ciwutil$(C)

# Standard headers
CIH   =ci.h ciconf.h cidebug.h ciinfo.h cipriv.h cistruct.h

# Machine generated headers
CICONF=cilibconf.h

# Headers callers use
CIINC= ci.h ciconf.h ciusr.h $(CICONF)

# Headers the CI library uses
CIHDRS=$(CIH) $(CICONF) ciusr.h

CIOBJS=ci$(O) cierror$(O) ciget$(O) cimem$(O) \
	ciset$(O) citrans$(O) ciwio$(O) ciwrite$(O) \
	ciwtran$(O) ciwutil$(O)

PROGSRCS= pnm2cim$(C)
PROGHDRS=
PROGDOCS=
PROGOBJS= pnm2cim$(O)

OBJS    = $(PROGOBJS) $(CIOBJS) $(ZOBJS)

# implicit make rules -------------------------------------------------------

.c$(O):
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

# dependencies

all: pnm2cim$(E)

pnm2cim$(E): $(OBJS)
	$(LD) -o pnm2cim$(E) $(OBJS)

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
	$(RM) pnm2cim$(O)
	$(RM) pnm2cim$(E)
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

pnm2cim.c: $(PROGSRC)/pnm2ci.c
	$(RM) $@
	$(COPY) $(PROGSRC)/pnm2ci.c $@

# End of makefile for pnm2cim

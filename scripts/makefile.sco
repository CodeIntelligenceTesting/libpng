# makefile for SCO OSr5  ELF and Unixware 7 with Native cc
# Contributed by Mike Hopkirk (hops at sco.com) modified from Makefile.lnx
#   force ELF build dynamic linking, SONAME setting in lib and RPATH in app
# Copyright (C) 2018-2025 Cosmin Truta
# Copyright (C) 2002, 2006, 2010-2014 Glenn Randers-Pehrson
# Copyright (C) 1998 Greg Roelofs
# Copyright (C) 1996, 1997 Andreas Dilger
#
# This code is released under the libci license.
# For conditions of distribution and use, see the disclaimer
# and license in ci.h

# Library name:
LIBNAME=libci16
CIMAJ=16

# Shared library names:
LIBSO=$(LIBNAME).so
LIBSOMAJ=$(LIBNAME).so.$(CIMAJ)

# Utilities:
CC=cc
AR=ar
RANLIB=echo
LN_SF=ln -f -s
CP=cp
RM_F=/bin/rm -f

# Where the zlib library and include files are located
#ZLIBLIB=/usr/local/lib
#ZLIBINC=/usr/local/include
ZLIBLIB=../zlib
ZLIBINC=../zlib

CPPFLAGS=-I$(ZLIBINC)
CFLAGS=-dy -belf -O3
ARFLAGS=rc
LDFLAGS=-L. -L$(ZLIBLIB) -lci16 -lz -lm

# Pre-built configuration
# See scripts/cilibconf.mak for more options
CILIBCONF_H_PREBUILT = scripts/cilibconf.h.prebuilt

OBJS = ci.o cierror.o ciget.o cimem.o cipread.o \
       ciread.o cirio.o cirtran.o cirutil.o ciset.o \
       citrans.o ciwio.o ciwrite.o ciwtran.o ciwutil.o

OBJSDLL = $(OBJS:.o=.pic.o)

.SUFFIXES:      .c .o .pic.o

.c.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $*.c

.c.pic.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -KPIC -o $@ $*.c

all: libci.a $(LIBSO) citest

cilibconf.h: $(CILIBCONF_H_PREBUILT)
	$(CP) $(CILIBCONF_H_PREBUILT) $@

libci.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(RANLIB) $@

$(LIBSO): $(LIBSOMAJ)
	$(LN_SF) $(LIBSOMAJ) $(LIBSO)

$(LIBSOMAJ): $(OBJSDLL)
	$(CC) -G  -Wl,-h,$(LIBSOMAJ) -o $(LIBSOMAJ) \
	 $(OBJSDLL)

citest: citest.o $(LIBSO)
	LD_RUN_PATH=.:$(ZLIBLIB) $(CC) -o citest $(CFLAGS) citest.o $(LDFLAGS)

test: citest
	./citest

install:
	@echo "The $@ target is no longer supported by this makefile."
	@false

install-static:
	@echo "The $@ target is no longer supported by this makefile."
	@false

install-shared:
	@echo "The $@ target is no longer supported by this makefile."
	@false

clean:
	$(RM_F) *.o libci.a citest ciout.ci
	$(RM_F) $(LIBSO) $(LIBSOMAJ)* citest-static cilibconf.h

# DO NOT DELETE THIS LINE -- make depend depends on it.

ci.o      ci.pic.o:      ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cierror.o cierror.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciget.o   ciget.pic.o:   ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cimem.o   cimem.pic.o:   ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cipread.o cipread.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciread.o  ciread.pic.o:  ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cirio.o   cirio.pic.o:   ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cirtran.o cirtran.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
cirutil.o cirutil.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciset.o   ciset.pic.o:   ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
citrans.o citrans.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciwio.o   ciwio.pic.o:   ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciwrite.o ciwrite.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciwtran.o ciwtran.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h
ciwutil.o ciwutil.pic.o: ci.h ciconf.h cilibconf.h cipriv.h cistruct.h ciinfo.h cidebug.h

citest.o: ci.h ciconf.h cilibconf.h

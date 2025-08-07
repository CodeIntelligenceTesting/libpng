# Makefile for CiMinus (ci2pnm and pnm2ci)
# Linux / Unix

#CC = cc
CC = gcc
LD = $(CC)

RM = rm -f

CIINC = -I../..
CILIB_SHARED = -L../.. -lci
CILIB_STATIC = ../../libci.a

# Uncomment the following if you have a custom zlib build at ../../../zlib
#ZINC = -I../../../zlib
#ZLIB_SHARED = -L../../../zlib -lz
#ZLIB_STATIC = ../../../zlib/libz.a

# Use the system zlib otherwise
ZLIB_SHARED = -lz
ZLIB_STATIC = -lz

CPPFLAGS = $(CIINC) $(ZINC)
CFLAGS =
LDFLAGS =
LIBS_SHARED = $(CILIB_SHARED) $(ZLIB_SHARED)
LIBS_STATIC = $(CILIB_STATIC) $(ZLIB_STATIC)

EXEEXT =
#EXEEXT = .exe

# dependencies

all: ci2pnm$(EXEEXT) pnm2ci$(EXEEXT) ci2pnm-static$(EXEEXT) pnm2ci-static$(EXEEXT)

ci2pnm.o: ci2pnm.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) ci2pnm.c

pnm2ci.o: pnm2ci.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) pnm2ci.c

ci2pnm$(EXEEXT): ci2pnm.o
	$(LD) $(LDFLAGS) -o ci2pnm$(EXEEXT) ci2pnm.o $(LIBS_SHARED) -lm

pnm2ci$(EXEEXT): pnm2ci.o
	$(LD) $(LDFLAGS) -o pnm2ci$(EXEEXT) pnm2ci.o $(LIBS_SHARED) -lm

ci2pnm-static$(EXEEXT): ci2pnm.o
	$(LD) $(LDFLAGS) -o ci2pnm-static$(EXEEXT) ci2pnm.o $(LIBS_STATIC) -lm

pnm2ci-static$(EXEEXT): pnm2ci.o
	$(LD) $(LDFLAGS) -o pnm2ci-static$(EXEEXT) pnm2ci.o $(LIBS_STATIC) -lm

clean:
	$(RM) ci2pnm.o
	$(RM) pnm2ci.o
	$(RM) ci2pnm$(EXEEXT)
	$(RM) pnm2ci$(EXEEXT)
	$(RM) ci2pnm-static$(EXEEXT)
	$(RM) pnm2ci-static$(EXEEXT)

# End of makefile for ci2pnm / pnm2ci

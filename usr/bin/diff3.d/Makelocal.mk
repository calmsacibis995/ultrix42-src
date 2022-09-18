# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
include $(GMAKEVARS)
#
#	Makefile	4.1	83/02/10

CFLAGS=-O

all: diff3

diff3: diff3.o
	cc -o diff3 diff3.o

diff3.o: ../diff3.c
	$(CCCMD) ../diff3.c

install:
	install -c -s diff3 $(DESTROOT)/usr/lib
	install -c ../diff3.sh $(DESTROOT)/usr/bin/diff3

include $(GMAKERULES)

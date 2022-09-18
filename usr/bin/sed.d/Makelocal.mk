# @(#)Makelocal.mk	4.1	(ULTRIX) 	7/17/90
#
#	Makefile	3.1	87/03/05
#
include $(GMAKEVARS)
DESTLIST=$(DESTROOT)/bin $(DESTROOT)/usr/bin
CFLAGS=-O -w
AOUT=sed
OBJS= sed0.o sed1.o

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h

pretools tools1 tools2: sed
pretools tools1 tools2 install:
	install -c -s sed $(DESTROOT)/bin

include $(GMAKERULES)

#  @(#)Makefile	1.2  ULTRIX  3/08/89

include $(GMAKEVARS)

DESTROOT=

CFLAGS=-O -DSUBR

all: makespt.o

makespt.o: makespt.c

include $(GMAKERULES)

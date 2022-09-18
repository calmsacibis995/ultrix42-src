#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

#	@(#)Makefile	2.1	(Berkeley)	12/10/85
# optional flags are: MEASURE TESTING DEBUG

include $(GMAKEVARS)

all:	cksum.o in_checksum.o

cksum.o:	cksum.c
in_checksum.o:	in_checksum.c

include $(GMAKERULES)


# @(#)Makelocal.mk	4.1 ULTRIX	7/17/90

include $(GMAKEVARS)

LDEFSLOCAL=../_$(MACHINE).b/ldefs.c

AOUT=	lex

OBJS=	lmain.o parser.o sub1.o sub2.o header.o

lmain.o:	lmain.c
sub1.o:		sub1.c
sub2.o:		sub2.c
header.o:	header.c

parser.o:	parser.y ldefs.c

install:
	$(INSTALL) -c -s lex $(DESTROOT)/usr/bin

include $(GMAKERULES)

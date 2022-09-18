#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS=	-O -Y

AOUT=	csplit

OBJS=	csplit.o

csplit:	csplit.o
csplit.o:	csplit.c

install:
	$(INSTALL) -c -s csplit $(DESTROOT)/usr/bin/csplit

include $(GMAKERULES)

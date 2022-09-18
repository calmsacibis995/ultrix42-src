#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ar11

OBJS=	ar11.o

ar11:	ar11.o
ar11.o:	ar11.c

install:
	$(INSTALL) -c -s ar11 $(DESTROOT)/usr/bin/ar11

include $(GMAKERULES)

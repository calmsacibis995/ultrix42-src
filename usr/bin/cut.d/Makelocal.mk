#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	cut

OBJS=	cut.o

cut:	cut.o
cut.o:	cut.c

install:
	$(INSTALL) -c -s cut $(DESTROOT)/usr/bin/cut

include $(GMAKERULES)

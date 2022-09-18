#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc

AOUT=	elcsd

OBJS=	elcsd.o

elcsd.o:	elcsd.c

install:
	$(INSTALL) -c -s elcsd $(DESTROOT)/etc/elcsd


include $(GMAKERULES)

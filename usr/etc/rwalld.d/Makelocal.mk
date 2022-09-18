#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	rwalld

OBJS=	rwalld.o

rwalld.o:	rwalld.c

install:
	$(INSTALL) -c -s rwalld $(DESTROOT)/usr/etc/rwalld

include $(GMAKERULES)

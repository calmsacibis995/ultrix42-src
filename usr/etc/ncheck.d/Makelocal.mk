#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	ncheck

OBJS=	ncheck.o

ncheck.o:	ncheck.c

install:
	$(INSTALL) -c -s ncheck $(DESTROOT)/etc/ncheck

include $(GMAKERULES)

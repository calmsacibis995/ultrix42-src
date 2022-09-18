#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	rzdisk

OBJS=	rzdisk.o

rzdisk.o:	rzdisk.c

install:
	$(INSTALL) -c -s rzdisk $(DESTROOT)/bin/rzdisk
	$(LN) -s ../bin/rzdisk $(DESTROOT)/etc/rzdisk

include $(GMAKERULES)

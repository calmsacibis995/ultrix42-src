#  @(#)Makelocal.mk	4.1	ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	rarpd

OBJS=	rarpd.o

rarpd:		rarpd.o
rarpd.o:	rarpd.c

install:
	$(INSTALL) -c -m 755 -s rarpd $(DESTROOT)/usr/etc/rarpd

include $(GMAKERULES)

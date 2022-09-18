#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	rdt

OBJS=	rdt.o

rdt.o:	rdt.c

install:
	$(INSTALL) -c -s rdt $(DESTROOT)/usr/etc/rdt
	$(RM) $(DESTROOT)/etc/rdt
	$(LN) -s ../usr/etc/rdt $(DESTROOT)/etc/rdt

include $(GMAKERULES)

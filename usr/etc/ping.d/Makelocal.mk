#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	ping

OBJS=	ping.o

ping.o:	ping.c

install:
	$(INSTALL) -c -s -m 4755 -o root ping $(DESTROOT)/usr/etc/ping
	$(RM) $(DESTROOT)/etc/ping
	$(LN) -s ../usr/etc/ping $(DESTROOT)/etc/ping

include $(GMAKERULES)

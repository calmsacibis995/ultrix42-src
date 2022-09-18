#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	inetd

OBJS=	inetd.o

inetd.o:	inetd.c

install:
	$(INSTALL) -c -s inetd $(DESTROOT)/usr/etc/inetd
	$(RM) $(DESTROOT)/etc/inetd
	$(LN) -s ../usr/etc/inetd $(DESTROOT)/etc/inetd

include $(GMAKERULES)

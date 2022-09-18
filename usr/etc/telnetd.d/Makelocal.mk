#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	telnetd

OBJS=	telnetd.o

telnetd.o:	telnetd.c

install:
	$(INSTALL) -c -s telnetd $(DESTROOT)/usr/etc/telnetd
	$(RM) $(DESTROOT)/etc/telnetd
	$(LN) -s ../usr/etc/telnetd $(DESTROOT)/etc/telnetd

include $(GMAKERULES)

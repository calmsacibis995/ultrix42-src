#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	arp

OBJS=	arp.o

arp.o:	arp.c

install:
	$(INSTALL) -c -m 2755 -g kmem -s $(AOUT) $(DESTROOT)/usr/etc/$(AOUT)
	$(RM) $(DESTROOT)/etc/$(AOUT)
	$(LN) -s ../usr/etc/$(AOUT) $(DESTROOT)/etc/$(AOUT)

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	portmap

OBJS=	portmap.o

portmap.o:	portmap.c

install:
	$(INSTALL) -c -s portmap $(DESTROOT)/usr/etc/portmap
	$(RM) $(DESTROOT)/etc/portmap
	$(LN) -s ../usr/etc/portmap $(DESTROOT)/etc/portmap

include $(GMAKERULES)

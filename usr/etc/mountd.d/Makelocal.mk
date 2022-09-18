#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	mountd

OBJS=	mountd.o

mountd.o:	mountd.c

install:
	$(INSTALL) -c -s mountd $(DESTROOT)/usr/etc/mountd
	$(RM) $(DESTROOT)/etc/mountd
	$(LN) -s ../usr/etc/mountd $(DESTROOT)/etc/mountd

include $(GMAKERULES)

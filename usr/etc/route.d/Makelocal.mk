#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	route

OBJS=	route.o

route.o:	route.c

install:
	$(INSTALL) -c -s route $(DESTROOT)/usr/etc/route
	$(RM) $(DESTROOT)/etc/route
	$(LN) -s ../usr/etc/route $(DESTROOT)/etc/route

include $(GMAKERULES)

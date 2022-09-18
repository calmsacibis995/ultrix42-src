#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	vipw

OBJS=	vipw.o

vipw.o:	vipw.c

install:
	$(INSTALL) -c -s vipw $(DESTROOT)/usr/etc/vipw
	$(RM) $(DESTROOT)/etc/vipw
	$(LN) -s ../usr/etc/vipw $(DESTROOT)/etc/vipw

include $(GMAKERULES)

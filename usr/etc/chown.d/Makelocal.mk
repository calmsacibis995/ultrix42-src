#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	chown

OBJS=	chown.o

chown.o:	chown.c

install:
	$(INSTALL) -c -s chown $(DESTROOT)/usr/etc/chown
	$(RM) $(DESTROOT)/etc/chown
	$(LN) -s ../usr/etc/chown $(DESTROOT)/etc/chown

include $(GMAKERULES)

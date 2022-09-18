#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	bscconfig

OBJS=	bscconfig.o

bscconfig.o:	bscconfig.c

install:
	$(INSTALL) -c -s bscconfig $(DESTROOT)/usr/etc/bscconfig
	$(RM) $(DESTROOT)/etc/bscconfig
	$(LN) -s ../usr/etc/bscconfig $(DESTROOT)/etc/bscconfig

include $(GMAKERULES)

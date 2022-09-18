#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/

AOUT=	pfconfig

OBJS=	pfconfig.o

pfconfig.o:	pfconfig.c

install:
	$(INSTALL) -c -s -m 755 pfconfig $(DESTROOT)/usr/etc/pfconfig

include $(GMAKERULES)

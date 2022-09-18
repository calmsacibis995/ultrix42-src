#  @(#)Makelocal.mk	4.1  ULTRIX  2/28/91

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUTS=	kvar

kvar:		kvar.o

kvar.o:		kvar.c 

install:
	$(INSTALL) -c -m 711 -g system -s kvar $(DESTROOT)/usr/etc/kvar

include $(GMAKERULES)

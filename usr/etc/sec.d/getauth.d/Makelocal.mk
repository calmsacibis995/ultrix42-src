#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/sec

AOUTS=	getauth rmauth setauth

LOADLIBES=	-lauth

getauth:	getauth.o
rmauth:		rmauth.o
setauth:	setauth.o
getauth.o:	getauth.c
rmauth.o:	rmauth.c
setauth.o:	setauth.c

install:
	$(INSTALL) -c -s -m 500 getauth $(DESTROOT)/usr/etc/sec/getauth
	$(INSTALL) -c -s -m 500 rmauth $(DESTROOT)/usr/etc/sec/rmauth
	$(INSTALL) -c -s -m 500 setauth $(DESTROOT)/usr/etc/sec/setauth

include $(GMAKERULES)

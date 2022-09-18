#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	yppasswd

OBJS=	yppasswd.o

yppasswd:	yppasswd.o
yppasswd.o:	yppasswd.c

install:
	$(INSTALL) -c -s yppasswd $(DESTROOT)/usr/bin/yppasswd

include $(GMAKERULES)

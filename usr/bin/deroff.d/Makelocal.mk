#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	deroff

OBJS=	deroff.o

deroff:	deroff.o
deroff.o:	deroff.c

install:
	$(INSTALL) -c -s deroff $(DESTROOT)/usr/bin/deroff

include $(GMAKERULES)

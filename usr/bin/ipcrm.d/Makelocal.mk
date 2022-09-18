#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ipcrm

OBJS=	ipcrm.o

ipcrm:	ipcrm.o
ipcrm.o:	ipcrm.c

install:
	$(INSTALL) -c -s ipcrm $(DESTROOT)/usr/bin/ipcrm

include $(GMAKERULES)

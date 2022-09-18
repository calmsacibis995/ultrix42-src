#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	comm

OBJS=	comm.o

comm:	comm.o
comm.o:	comm.c

install:
	$(INSTALL) -c -s comm $(DESTROOT)/usr/bin/comm

include $(GMAKERULES)

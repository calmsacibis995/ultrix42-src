#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	sleep

OBJS=	sleep.o

sleep:	sleep.o
sleep.o:	sleep.c

install:
	$(INSTALL) -c -s sleep $(DESTROOT)/usr/bin/sleep

include $(GMAKERULES)

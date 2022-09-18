# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib $(DESTROOT)/usr/bin

AOUT=calendar

OBJS=calendar.o

calendar.o: calendar.c

install:
	$(INSTALL) -c -s calendar $(DESTROOT)/usr/lib/calendar
	$(INSTALL) -c -m 755 ../calendar.sh $(DESTROOT)/usr/bin/calendar

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	cron

OBJS=	cron.o

cron.o:	cron.c

install:
	$(INSTALL) -c -s cron $(DESTROOT)/usr/etc/cron
	$(RM) $(DESTROOT)/etc/cron
	$(LN) -s ../usr/etc/cron $(DESTROOT)/etc/cron

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	renice

OBJS=	renice.o

renice.o:	renice.c

install:
	$(INSTALL) -c -s renice $(DESTROOT)/usr/etc/renice
	$(RM) $(DESTROOT)/etc/renice
	$(LN) -s ../usr/etc/renice $(DESTROOT)/etc/renice

include $(GMAKERULES)

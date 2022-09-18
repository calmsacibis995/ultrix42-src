#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	rmt

OBJS=	rmt.o

rmt.o:	rmt.c

install:
	$(INSTALL) -c -s rmt $(DESTROOT)/usr/etc/rmt
	$(RM) $(DESTROOT)/etc/rmt
	$(LN) -s ../usr/etc/rmt $(DESTROOT)/etc/rmt

include $(GMAKERULES)

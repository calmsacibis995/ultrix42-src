#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	rwhod

OBJS=	rwhod.o

rwhod.o:	rwhod.c

install:
	$(INSTALL) -c -s rwhod $(DESTROOT)/usr/etc/rwhod
	$(RM) $(DESTROOT)/etc/rwhod
	$(LN) -s ../usr/etc/rwhod $(DESTROOT)/etc/rwhod

include $(GMAKERULES)

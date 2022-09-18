#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	rdate

OBJS=	rdate.o

rdate.o:	rdate.c

install:
	$(INSTALL) -c -s rdate $(DESTROOT)/usr/etc/rdate
	$(RM) $(DESTROOT)/etc/rdate
	$(LN) -s ../usr/etc/rdate $(DESTROOT)/etc/rdate

include $(GMAKERULES)

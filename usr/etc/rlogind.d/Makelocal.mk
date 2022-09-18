#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	rlogind

OBJS=	rlogind.o

rlogind.o:	rlogind.c

install:
	$(INSTALL) -c -s rlogind $(DESTROOT)/usr/etc/rlogind
	$(RM) $(DESTROOT)/etc/rlogind
	$(LN) -s ../usr/etc/rlogind $(DESTROOT)/etc/rlogind

include $(GMAKERULES)

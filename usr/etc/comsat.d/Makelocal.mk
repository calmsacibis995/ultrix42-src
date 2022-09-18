#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	comsat

OBJS=	comsat.o

comsat.o:	comsat.c

install:
	$(INSTALL) -c -s comsat $(DESTROOT)/usr/etc/comsat
	$(RM) $(DESTROOT)/etc/comsat
	$(LN) -s ../usr/etc/comsat $(DESTROOT)/etc/comsat

include $(GMAKERULES)

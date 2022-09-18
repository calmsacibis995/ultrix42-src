#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	rmail

OBJS=	rmail.o

rmail:	rmail.o
rmail.o:	rmail.c

install:
	$(INSTALL) -c -s rmail $(DESTROOT)/usr/bin/rmail
	$(RM) $(DESTROOT)/bin/rmail
	$(LN) -s ../usr/bin/rmail $(DESTROOT)/bin/rmail

include $(GMAKERULES)

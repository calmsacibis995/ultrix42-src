#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tee

OBJS=	tee.o

tee:	tee.o
tee.o:	tee.c

install:
	$(INSTALL) -c -s tee $(DESTROOT)/usr/bin/tee
	$(RM) $(DESTROOT)/bin/tee
	$(LN) -s ../usr/bin/tee $(DESTROOT)/bin/tee

include $(GMAKERULES)

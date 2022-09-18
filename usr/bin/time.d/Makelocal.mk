#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	time

OBJS=	time.o

time:	time.o
time.o:	time.c

install:
	$(INSTALL) -c -s time $(DESTROOT)/usr/bin/time
	$(RM) $(DESTROOT)/bin/time
	$(LN) -s ../usr/bin/time $(DESTROOT)/bin/time

include $(GMAKERULES)

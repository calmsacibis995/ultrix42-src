#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	logname

OBJS=	logname.o

logname:	logname.o
logname.o:	logname.c

install:
	$(INSTALL) -c -s logname $(DESTROOT)/usr/bin/logname
	$(RM) $(DESTROOT)/bin/logname
	$(LN) -s ../usr/bin/logname $(DESTROOT)/bin/logname

include $(GMAKERULES)

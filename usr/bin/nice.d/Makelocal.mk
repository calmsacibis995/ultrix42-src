#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	nice

OBJS=	nice.o

nice:	nice.o
nice.o:	nice.c

install:
	$(INSTALL) -c -s nice $(DESTROOT)/usr/bin/nice
	$(RM) $(DESTROOT)/bin/nice
	$(LN) -s ../usr/bin/nice $(DESTROOT)/bin/nice

include $(GMAKERULES)

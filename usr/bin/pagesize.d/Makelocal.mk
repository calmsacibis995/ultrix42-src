#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	pagesize

OBJS=	pagesize.o

pagesize:	pagesize.o
pagesize.o:	pagesize.c

install:
	$(INSTALL) -c -s pagesize $(DESTROOT)/usr/bin/pagesize
	$(RM) $(DESTROOT)/bin/pagesize
	$(LN) -s ../usr/bin/pagesize $(DESTROOT)/bin/pagesize

include $(GMAKERULES)

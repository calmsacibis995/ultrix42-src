#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	du

OBJS=	du.o

du:	du.o
du.o:	du.c

install:
	$(INSTALL) -c -s du $(DESTROOT)/usr/bin/du
	$(RM) $(DESTROOT)/bin/du
	$(LN) -s ../usr/bin/du $(DESTROOT)/bin/du

include $(GMAKERULES)

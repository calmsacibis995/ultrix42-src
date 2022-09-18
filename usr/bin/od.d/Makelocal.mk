#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	od

OBJS=	od.o

od:	od.o
od.o:	od.c

install:
	$(INSTALL) -c -s od $(DESTROOT)/usr/bin/od
	$(RM) $(DESTROOT)/bin/od
	$(LN) -s ../usr/bin/od $(DESTROOT)/bin/od

include $(GMAKERULES)

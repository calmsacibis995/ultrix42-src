#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	pr

OBJS=	pr.o

pr:	pr.o
pr.o:	pr.c

install:
	$(INSTALL) -c -s pr $(DESTROOT)/usr/bin/pr
	$(RM) $(DESTROOT)/bin/pr
	$(LN) -s ../usr/bin/pr $(DESTROOT)/bin/pr

include $(GMAKERULES)

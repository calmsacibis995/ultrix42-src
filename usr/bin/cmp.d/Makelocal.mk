#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	cmp

OBJS=	cmp.o

cmp:	cmp.o
cmp.o:	cmp.c

install:
	$(INSTALL) -c -s cmp $(DESTROOT)/usr/bin/cmp
	$(RM) $(DESTROOT)/bin/cmp
	$(LN) -s ../usr/bin/cmp $(DESTROOT)/bin/cmp

include $(GMAKERULES)

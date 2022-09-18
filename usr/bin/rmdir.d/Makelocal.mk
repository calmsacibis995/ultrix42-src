#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	rmdir

OBJS=	rmdir.o

rmdir:	rmdir.o
rmdir.o:	rmdir.c

install:
	$(INSTALL) -c -s rmdir $(DESTROOT)/usr/bin/rmdir
	$(RM) $(DESTROOT)/bin/rmdir
	$(LN) -s ../usr/bin/rmdir $(DESTROOT)/bin/rmdir

include $(GMAKERULES)

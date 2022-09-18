#  @(#)Makelocal.mk	2.1  ULTRIX  4/12/89

include $(GMAKEVARS)

AOUT=	uname

OBJS=	uname.o

uname:	uname.o
uname.o:	uname.c

install:
	$(INSTALL) -c -s -m 4755 uname $(DESTROOT)/usr/bin/uname
	$(RM) $(DESTROOT)/bin/uname
	$(LN) -s ../usr/bin/uname $(DESTROOT)/bin/uname

include $(GMAKERULES)

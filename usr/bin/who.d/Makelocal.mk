#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	who

OBJS=	who.o

who:	who.o
who.o:	who.c

install:
	$(INSTALL) -c -s who $(DESTROOT)/usr/bin/who
	$(RM) $(DESTROOT)/bin/who
	$(LN) -s ../usr/bin/who $(DESTROOT)/bin/who

include $(GMAKERULES)

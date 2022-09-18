#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	line

OBJS=	line.o

line:	line.o
line.o:	line.c

install:
	$(INSTALL) -c -s line $(DESTROOT)/usr/bin/line
	$(RM) $(DESTROOT)/bin/line
	$(LN) -s ../usr/bin/line $(DESTROOT)/bin/line

include $(GMAKERULES)

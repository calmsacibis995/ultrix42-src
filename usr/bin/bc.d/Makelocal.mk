#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=bc

OBJS=bc.o

bc:	bc.o
bc.o:	bc.y

install:
	$(INSTALL) -c -s bc $(DESTROOT)/usr/bin/bc

include $(GMAKERULES)

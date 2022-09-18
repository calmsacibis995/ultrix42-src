#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	expr

OBJS=	expr.o

expr.o:	expr.y

install:
	$(INSTALL) -c -s expr $(DESTROOT)/bin/expr

include $(GMAKERULES)

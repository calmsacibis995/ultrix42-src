#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/etc

CDEFINES=-DLOCALNET=0
YFLAGS=-d

AOUT=htable

OBJS=htable.o parse.o scan.o


htable.o: htable.c htable.h y.tab.h
scan.o:	scan.l htable.h y.tab.h y.tab.h
parse.o: parse.y htable.h y.tab.h

y.tab.h:
	$(YACC) $(YFLAGS) ../parse.y
	$(RM) y.tab.c

install:
	$(INSTALL) -c -s htable ${DESTROOT}/etc/htable

include $(GMAKERULES)

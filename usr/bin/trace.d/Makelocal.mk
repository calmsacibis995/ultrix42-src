#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	trace

OBJS=	trace.o

trace:	trace.o
trace.o:	trace.c

install:
	$(INSTALL) -c -s trace $(DESTROOT)/usr/bin/trace

include $(GMAKERULES)

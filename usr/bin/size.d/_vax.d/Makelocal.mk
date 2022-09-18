# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=size
OBJS=size.o
LDFLAGS=-s

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c size ${DESTROOT}/usr/bin/size
	$(RM) ${DESTROOT}/bin/size
	ln -s ../usr/bin/size ${DESTROOT}/bin/size

size.o:	size.c

include $(GMAKERULES)

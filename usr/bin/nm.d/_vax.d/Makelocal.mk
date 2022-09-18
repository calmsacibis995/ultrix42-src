# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=nm
OBJS=nm.o
LDFLAGS=-s

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c nm ${DESTROOT}/usr/bin/nm
	$(RM) ${DESTROOT}/bin/nm
	ln -s ../usr/bin/nm ${DESTROOT}/bin/nm

nm.o:	nm.c

include $(GMAKERULES)

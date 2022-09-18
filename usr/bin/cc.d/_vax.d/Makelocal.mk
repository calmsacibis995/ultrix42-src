# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=cc
OBJS=cc.o
LDFLAGS= -s

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c cc ${DESTROOT}/usr/bin/cc
	$(RM) ${DESTROOT}/bin/cc
	ln -s ../usr/bin/cc ${DESTROOT}/bin/cc

cc.o:	cc.c

include $(GMAKERULES)

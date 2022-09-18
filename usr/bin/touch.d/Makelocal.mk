#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
include $(GMAKEVARS)
AOUT=touch
OBJS=touch.o
CFLAGS=-O -Y
LDFLAGS=${CFLAGS} -s

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c touch ${DESTROOT}/usr/bin/touch

touch.o:	touch.c

include $(GMAKERULES)

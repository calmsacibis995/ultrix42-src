# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90

include $(GMAKEVARS)

YFLAGS=-d

AOUT=	ratfor

OBJS=	r.o r0.o r1.o r2.o rio.o rlook.o rlex.o

r.o:		r.y
r0.o:		r0.c r.h
r1.o:		r1.c r.h
r2.o:		r2.c r.h
rio.o:		rio.c r.h
rlook.o:	rlook.c r.h
rlex.o:		rlex.c r.h

install:
	$(INSTALL) -c -s ratfor $(DESTROOT)/usr/bin/ratfor

include $(GMAKERULES)

# @(#)Makelocal.mk	4.1	(ULTRIX)	7/3/90

include $(GMAKEVARS)

CDEFINES=-DCOPYCODE -DC2
CFLAGS=-w -O
CINCLUDES=-I. -I.. -I$(SRCROOT)/usr/include

ASDIR = ../../as

AOUT=	c2

OBJS=	c20.o c21.o c22.o

c20.o:	c20.c c2.h
c21.o:	c21.c c2.h
c22.o:  c22.c c2.h instrs.c2

instrs.c2: $(ASDIR)/instrs
	($(ECHO) FLAVOR C2 ; $(CAT) $(ASDIR)/instrs) | \
		$(AWK) -f ${ASDIR}/instrs > instrs.c2

install:
	$(INSTALL) -c -s c2 $(DESTROOT)/usr/lib/c2

include $(GMAKERULES)

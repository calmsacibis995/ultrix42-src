#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=cxref

OBJS=cxr.o

cxr.o: cxr.c owner.h

install:
	$(INSTALL) -c -s cxref $(DESTROOT)/usr/bin/cxref

include $(GMAKERULES)


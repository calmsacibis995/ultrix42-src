#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=egrep

OBJS=egrep.o

egrep: egrep.o
egrep.o: egrep.y

install:
	$(INSTALL) -c -s egrep $(DESTROOT)/usr/bin/egrep

include $(GMAKERULES)

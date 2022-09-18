#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	mknod

OBJS=	mknod.o

mknod.o:	mknod.c

install:
	$(INSTALL) -c -s mknod $(DESTROOT)/bin/mknod
	$(RM) $(DESTROOT)/etc/mknod
	$(LN) -s ../bin/mknod $(DESTROOT)/etc/mknod

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	mkfs

OBJS=	mkfs.o

mkfs.o:	mkfs.c

install:
	$(INSTALL) -c -s mkfs $(DESTROOT)/bin/mkfs
	$(RM) $(DESTROOT)/etc/mkfs
	$(LN) -s ../bin/mkfs $(DESTROOT)/etc/mkfs

include $(GMAKERULES)

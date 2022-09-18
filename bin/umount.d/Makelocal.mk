#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	umount

OBJS=	umount.o

umount.o:	umount.c

install:
	$(INSTALL) -c -s umount $(DESTROOT)/bin/umount
	$(RM) $(DESTROOT)/etc/umount
	$(LN) -s ../bin/umount $(DESTROOT)/etc/umount

include $(GMAKERULES)

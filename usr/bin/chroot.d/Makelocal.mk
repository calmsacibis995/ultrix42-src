#  @(#)Makelocal.mk	4.3 ULTRIX  3/13/91

include $(GMAKEVARS)

AOUT=	chroot

OBJS=	chroot.o

chroot:	chroot.o
chroot.o:	chroot.c

install:
	$(INSTALL) -c -s -m 755 chroot $(DESTROOT)/usr/bin/chroot

include $(GMAKERULES)

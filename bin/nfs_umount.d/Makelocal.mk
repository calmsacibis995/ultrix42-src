#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	nfs_umount

OBJS=	nfs_umount.o

nfs_umount.o:	nfs_umount.c

install:
	$(INSTALL) -c -s nfs_umount $(DESTROOT)/bin/nfs_umount
	$(RM) $(DESTROOT)/etc/nfs_umount
	$(LN) -s ../bin/nfs_umount ${DESTROOT}/etc/nfs_umount


include $(GMAKERULES)

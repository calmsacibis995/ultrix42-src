#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	nfs_mount

OBJS=	nfs_mount.o

nfs_mount.o:	nfs_mount.c

install:
	$(INSTALL) -c -s nfs_mount $(DESTROOT)/bin/nfs_mount
	$(RM) $(DESTROOT)/etc/nfs_mount
	$(LN) -s ../bin/nfs_mount $(DESTROOT)/etc/nfs_mount

include $(GMAKERULES)

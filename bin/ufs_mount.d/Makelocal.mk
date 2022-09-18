#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	ufs_mount

OBJS=	ufs_mount.o

ufs_mount.o:	ufs_mount.c

install:
	$(INSTALL) -c -s ufs_mount $(DESTROOT)/bin/ufs_mount
	$(RM) $(DESTROOT)/etc/ufs_mount
	$(LN) -s ../bin/ufs_mount $(DESTROOT)/etc/ufs_mount

include $(GMAKERULES)

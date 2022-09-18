#  @(#)Makelocal.mk	4.1  ULTRIX  11/9/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	cdfs_mount

OBJS=	cdfs_mount.o

cdfs_mount.o:	cdfs_mount.c

install:
	$(INSTALL) -c -s cdfs_mount $(DESTROOT)/bin/cdfs_mount
	$(RM) $(DESTROOT)/etc/cdfs_mount
	$(LN) -s ../bin/cdfs_mount $(DESTROOT)/etc/cdfs_mount

include $(GMAKERULES)

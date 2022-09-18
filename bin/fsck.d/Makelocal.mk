# @(#)Makelocal.mk	4.2	(ULTRIX)	10/12/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc
KERNALSRC=../../../sys/fs/ufs

HDRS=	fsck.h

AOUTS=	fsck fsck.sas

OBJS=	$(LOCOBJS) $(SYSOBJS) inode.o inode.sas.o
LOCOBJS=dir.o main.o pass1.o pass1b.o pass2.o \
        pass3.o pass4.o pass5.o setup.o utilities.o
SYSOBJS=ufs_subr.o ufs_tables.o

# special case stuff for STANDALONE version (smaller)
inode.sas.o: inode.sas.c
inode.sas:	inode.sas.o
fsck:	$(LOCOBJS) $(SYSOBJS) inode.o
fsck.sas: $(LOCOBJS) $(SYSOBJS) inode.sas.o

inode.sas.c: inode.c
	-rm -f inode.sas.c
	cp ../inode.c inode.sas.c

inode.sas.o:
	$(CCCMD) -DSTANDALONE inode.sas.c

dir.o:		dir.c $(HDRS)

inode.o:	inode.c $(HDRS)
inode.sas.o:	inode.c $(HDRS)

main.o:		main.c $(HDRS)

pass1.o:	pass1.c $(HDRS)

pass1b.o:	pass1b.c $(HDRS)

pass2.o:	pass2.c $(HDRS)

pass3.o:	pass3.c $(HDRS)

pass4.o:	pass4.c $(HDRS)

pass5.o:	pass5.c $(HDRS)

setup.o:	setup.c $(HDRS)

utilities.o:	utilities.c $(HDRS)

ufs_subr.o: $(KERNALSRC)/ufs_subr.c
	$(CCCMD) $(KERNALSRC)/ufs_subr.c

ufs_tables.o: $(KERNALSRC)/ufs_tables.c
	$(CCCMD) $(KERNALSRC)/ufs_tables.c

install:
	$(INSTALL) -c -s -m 4755 fsck $(DESTROOT)/bin/fsck
	$(INSTALL) -c -s -m 4755 fsck.sas $(DESTROOT)/etc/fsck.sas
	$(RM) $(DESTROOT)/etc/fsck
	$(LN) -s ../bin/fsck $(DESTROOT)/etc/fsck

include $(GMAKERULES)

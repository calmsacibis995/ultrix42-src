#	Makelocal.mk -
#		sys/dist/dev.d/_vax.d Makefile.
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New.
#
#	001	15-mar-1989	map (Mark A. Parenti)
#	Remove DESTLIST.  Because the install rule here has dependencies,
#	they are executed BEFORE the DESTLIST rule. As a result, the ouput
#	directories did not exist.  The DESTLIST rule is now done in the
#	upper level Makelocal.mk to ensure that the directories exist before
#	the install rule here is executed.
#

include $(GMAKEVARS)

install:	makedev makedev.local diskless

makedev:
	$(INSTALL) -c -m 755 ../../MAKEDEV $(DESTROOT)/dev/MAKEDEV

makedev.local:
	> $(DESTROOT)/dev/MAKEDEV.local
	chmod 644 $(DESTROOT)/dev/MAKEDEV.local
	chgrp system $(DESTROOT)/dev/MAKEDEV.local
	/etc/chown root $(DESTROOT)/dev/MAKEDEV.local

diskless:
	$(INSTALL) -c -m 755 ../../MAKEDEV $(DESTROOT)/usr/diskless/dev/MAKEDEV
	(cd $(DESTROOT)/usr/diskless/dev; \
		./MAKEDEV console drum errlog kUmem kmem mem null \
			tty pty0 pty1)

include $(GMAKERULES)


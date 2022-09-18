#	Makelocal.mk -
#		sys/dist/dev.d/_mips.d Makefile.
#
#  @(#)Makelocal.mk	4.2	ULTRIX  9/4/90
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
# 	001	04-sep-1990	Tungning Cherng
#		Make only required devices for diskless clients
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
	    ./MAKEDEV  console drum errlog kUmem kmem mem null pty0 pty1 tty)

include $(GMAKERULES)


#  @(#)Makelocal.mk	4.2  ULTRIX  11/9/90

include $(GMAKEVARS)

SUBDIRS=cat.d cdfs_mount.d chmod.d chpt.d clri.d cp.d date.d dd.d  \
	dirname.d dump.d dumpfs.d echo.d ed.d expr.d false.d fsck.d \
	fsirand.d grep.d halt.d hostname.d icheck.d ifconfig.d init.d \
	kill.d ln.d ls.d mkdir.d mkfs.d mkmachine.d mknod.d mount.d \
	mt.d mv.d newfs.d nfs_mount.d nfs_umount.d ps.d pwd.d radisk.d \
	reboot.d restore.d rm.d rzdisk.d sh.d shutdown.d snapcopy.d \
	stty.d sync.d tar.d test.d true.d tunefs.d ufs_mount.d \
	umount.d 

include $(GMAKERULES)

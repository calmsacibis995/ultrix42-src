#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

CFLAGS=	-O -Y

OBJS=	_fcntl.o _getpgrp.o _ioctl.o _kill.o _lseek.o _mount.o \
	_open.o _ptrace.o _read.o _setpgrp.o _sigvec.o _umount.o \
	_wait.o _write.o

_fcntl.o:		_fcntl.s
_getpgrp.o:		_getpgrp.s
_ioctl.o:		_ioctl.s
_kill.o:		_kill.s
_lseek.o:		_lseek.s
_mount.o:		_mount.s
_open.o:		_open.s
_ptrace.o:		_ptrace.s
_read.o:		_read.s
_setpgrp.o:		_setpgrp.s
_sigvec.o:		_sigvec.s
_umount.o:		_umount.s
_wait.o:		_wait.s
_write.o:		_write.s

include $(GMAKERULES)

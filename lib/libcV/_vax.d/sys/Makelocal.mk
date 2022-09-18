#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	_fcntl.o _getpgrp.o _ioctl.o _kill.o _lseek.o _mount.o \
	_open.o _ptrace.o _read.o _setpgrp.o _sigvec.o _umount.o \
	_wait.o _write.o

_fcntl.o:		_fcntl.c
_getpgrp.o:		_getpgrp.c
_ioctl.o:		_ioctl.c
_kill.o:		_kill.c
_lseek.o:		_lseek.c
_mount.o:		_mount.c
_open.o:		_open.c
_ptrace.o:		_ptrace.c
_read.o:		_read.c
_setpgrp.o:		_setpgrp.c
_sigvec.o:		_sigvec.c
_umount.o:		_umount.c
_wait.o:		_wait.c
_write.o:		_write.c

$(OBJS):
	/lib/cpp -E -DPROF  ../$*.c | ${AS} -o $*.o
	-ld -x -r -o profiled/$*.o $*.o
	/lib/cpp -E  ../$*.c | ${AS} -o $*.o
	-ld -x -r -o gfloat/$*.o $*.o
	/lib/cpp -E  ../$*.c | ${AS} -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

include $(GMAKERULES)

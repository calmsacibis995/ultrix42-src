#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJSMISC=alarm.o gtty.o nice.o stty.o time.o

OBJS=	getpgrp.o ioctl.o kill.o \
	lseek.o mount.o open.o ptrace.o read.o \
	setpgrp.o times.o \
	umount.o wait.o write.o

include ../Makelocal_$(MACHINE).mk

alarm.o:		alarm.c
gtty.o:			gtty.c
nice.o:			nice.c
stty.o:			stty.c
time.o:			time.c
getpgrp.o:		getpgrp.c
ioctl.o:		ioctl.c
kill.o:			kill.c
lseek.o:		lseek.c
mount.o:		mount.c
open.o:			open.c
ptrace.o:		ptrace.c
read.o:			read.c
setpgrp.o:		setpgrp.c
times.o:		times.c
umount.o:		umount.c
wait.o:			wait.c
write.o:		write.c

alarm.c time.c:
	$(RM) $@
	$(LN) -s ../../../libc/gen/$@ $@

gtty.c nice.c stty.c:
	$(RM) $@
	$(LN) -s ../../../libc/compat-4.1/$@ $@

include $(GMAKERULES)

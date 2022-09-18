#  @(#)Makelocal.mk	4.2  ULTRIX  9/10/90

include $(GMAKEVARS)

OBJS1=	Ovfork.o Ovadvise.o \
	accept.o access.o acct.o adjtime.o audcntl.o audgen.o \
	bind.o brk.o \
	cerror.o chdir.o chmod.o chown.o chroot.o close.o connect.o \
	creat.o \
	dup.o dup2.o \
	execl.o execle.o exect.o execv.o execve.o _exit.o exportfs.o \
	fchmod.o fchown.o fcntl.o flock.o fork.o fstat.o fsync.o \
	ftruncate.o \
	getdirentries.o getdomainname.o getdtablesize.o getegid.o \
	geteuid.o getgid.o getgroups.o gethostid.o gethostname.o \
	getmnt.o getitimer.o getrlimit.o getpagesize.o getpeername.o \
	getpgrp.o getpid.o getppid.o getpriority.o getrusage.o \
	getsockname.o getsockopt.o getsysinfo.o gettimeofday.o \
	getuid.o \
	ioctl.o \
	kill.o killpg.o \
	link.o listen.o lseek.o lstat.o \
	mkdir.o mknod.o mount.o mprotect.o
OBJS2= msgctl.o msgget.o msgrcv.o msgsnd.o\
	nfs_biod.o nfs_getfh.o nfs_svc.o \
	open.o \
	pipe.o profil.o ptrace.o \
	quota.o \
	read.o readlink.o readv.o reboot.o recv.o recvfrom.o \
	recvmsg.o rename.o rmdir.o \
	sbrk.o select.o \
	semctl.o semget.o semop.o \
	send.o sendmsg.o sendto.o setdomainname.o setregid.o \
	setgroups.o sethostid.o sethostname.o setitimer.o setquota.o \
	setrlimit.o setpgrp.o setpriority.o setsid.o setsockopt.o \
	setsysinfo.o settimeofday.o setreuid.o \
	shmat.o shmctl.o shmdt.o shmget.o shmsys.o plock.o \
	shutdown.o sigblock.o sigpending.o sigpause.o \
	sigsetmask.o sigstack.o sigvec.o socket.o socketpair.o startcpu.o \
	stat.o stopcpu.o swapon.o symlink.o sync.o syscall.o \
	truncate.o \
	umask.o umount.o uname.o unlink.o ustat.o utimes.o \
	vhangup.o \
	wait.o waitpid.o wait3.o write.o writev.o mmap.o munmap.o

all: $(OBJS1) $(OBJS2)

Ovfork.o:		Ovfork.c
Ovadvise.o:		Ovadvise.c
accept.o:		accept.c
access.o:		access.c
acct.o:			acct.c
adjtime.o:		adjtime.c
audcntl.o:		audcntl.c
audgen.o:		audgen.c
bind.o:			bind.c
brk.o:			brk.c
cerror.o:		cerror.c
chdir.o:		chdir.c
chmod.o:		chmod.c
chown.o:		chown.c
chroot.o:		chroot.c
close.o:		close.c
connect.o:		connect.c
creat.o:		creat.c
dup.o:			dup.c
dup2.o:			dup2.c
execl.o:		execl.c
execle.o:		execle.c
exect.o:		exect.c
execv.o:		execv.c
execve.o:		execve.c
_exit.o:		_exit.c
exportfs.o:		exportfs.c
fchmod.o:		fchmod.c
fchown.o:		fchown.c
fcntl.o:		fcntl.c
flock.o:		flock.c
fork.o:			fork.c
fstat.o:		fstat.c
fsync.o:		fsync.c
ftruncate.o:		ftruncate.c
getdirentries.o:	getdirentries.c
getdomainname.o:	getdomainname.c
getdtablesize.o:	getdtablesize.c
getegid.o:		getegid.c
geteuid.o:		geteuid.c
getgid.o:		getgid.c
getgroups.o:		getgroups.c
gethostid.o:		gethostid.c
gethostname.o:		gethostname.c
getmnt.o:		getmnt.c
getitimer.o:		getitimer.c
getrlimit.o:		getrlimit.c
getpagesize.o:		getpagesize.c
getpeername.o:		getpeername.c
getpgrp.o:		getpgrp.c
getpid.o:		getpid.c
getppid.o:		getppid.c
getpriority.o:		getpriority.c
getrusage.o:		getrusage.c
getsockname.o:		getsockname.c
getsockopt.o:		getsockopt.c
getsysinfo.o:		getsysinfo.c
gettimeofday.o:		gettimeofday.c
getuid.o:		getuid.c
ioctl.o:		ioctl.c
kill.o:			kill.c
killpg.o:		killpg.c
link.o:			link.c
listen.o:		listen.c
lseek.o:		lseek.c
lstat.o:		lstat.c
mkdir.o:		mkdir.c
mknod.o:		mknod.c
mmap.o:			mmap.c
munmap.o:		munmap.c
mount.o:		mount.c
mprotect.o:		mprotect.c
msgctl.o:		msgctl.c
msgget.o:		msgget.c
msgrcv.o:		msgrcv.c
msgsnd.o:		msgsnd.c
nfs_biod.o:		nfs_biod.c
nfs_getfh.o:		nfs_getfh.c
nfs_svc.o:		nfs_svc.c
open.o:			open.c
pipe.o:			pipe.c
profil.o:		profil.c
ptrace.o:		ptrace.c
quota.o:		quota.c
read.o:			read.c
readlink.o:		readlink.c
readv.o:		readv.c
reboot.o:		reboot.c
recv.o:			recv.c
recvfrom.o:		recvfrom.c
recvmsg.o:		recvmsg.c
rename.o:		rename.c
rmdir.o:		rmdir.c
sbrk.o:			sbrk.c
select.o:		select.c
semctl.o:		semctl.c
semget.o:		semget.c
semop.o:		semop.c
send.o:			send.c
sendmsg.o:		sendmsg.c
sendto.o:		sendto.c
setdomainname.o:	setdomainname.c
setregid.o:		setregid.c
setgroups.o:		setgroups.c
sethostid.o:		sethostid.c
sethostname.o:		sethostname.c
setitimer.o:		setitimer.c
setquota.o:		setquota.c
setrlimit.o:		setrlimit.c
setpgrp.o:		setpgrp.c
setpriority.o:		setpriority.c
setsid.o:		setsid.c
setsockopt.o:		setsockopt.c
setsysinfo.o:		setsysinfo.c
settimeofday.o:		settimeofday.c
setreuid.o:		setreuid.c
shmat.o:		shmat.c
shmctl.o:		shmctl.c
shmdt.o:		shmdt.c
shmget.o:		shmget.c
shmsys.o:		shmsys.c
plock.o:		plock.c
shutdown.o:		shutdown.c
sigblock.o:		sigblock.c
sigpending.o:		sigpending.c
sigpause.o:		sigpause.c
sigsetmask.o:		sigsetmask.c
sigstack.o:		sigstack.c
sigvec.o:		sigvec.c
socket.o:		socket.c
socketpair.o:		socketpair.c
startcpu.o:		startcpu.c
stat.o:			stat.c
stopcpu.o:		stopcpu.c
swapon.o:		swapon.c
symlink.o:		symlink.c
sync.o:			sync.c
syscall.o:		syscall.c
truncate.o:		truncate.c
umask.o:		umask.c
umount.o:		umount.c
uname.o:		uname.c
unlink.o:		unlink.c
ustat.o:		ustat.c
utimes.o:		utimes.c
vhangup.o:		vhangup.c
wait.o:			wait.c
waitpid.o:		waitpid.c
wait3.o:		wait3.c
write.o:		write.c
writev.o:		writev.c

$(OBJS1) $(OBJS2):
	/lib/cpp -E -DPROF  ../$*.c | $(AS) -o $*.o
	-ld -x -r -o profiled/$*.o $*.o
	/lib/cpp -E  ../$*.c | $(AS) -o $*.o
	-ld -x -r -o gfloat/$*.o $*.o
	/lib/cpp -E  ../$*.c | $(AS) -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o

clean:	cleangfloat cleanprofiled

cleangfloat:
	$(RM) gfloat/*

cleanprofiled:
	$(RM) profiled/*

include $(GMAKERULES)

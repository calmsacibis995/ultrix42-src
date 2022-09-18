#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS1=	Ovadvise.o Ovfork.o _exit.o \
	accept.o access.o acct.o adjtime.o atomic_op.o audcntl.o audgen.o \
	bind.o brk.o \
	cachectl.o cacheflush.o cerror.o chdir.o chmod.o \
	chown.o chroot.o close.o connect.o creat.o \
	dup.o dup2.o \
	execl.o execle.o exect.o execv.o execve.o exportfs.o \
	fchmod.o fchown.o fcntl.o fixade.o flock.o fork.o \
	fp_sigintr.o fstat.o fsync.o ftruncate.o \
	getdirentries.o getdomainname.o getdtablesize.o getegid.o \
	geteuid.o getgid.o getgroups.o gethostid.o gethostname.o \
	getitimer.o getmnt.o getpagesize.o getpeername.o getpgrp.o \
	getpid.o getppid.o getpriority.o getrlimit.o getrusage.o \
	getsockname.o getsockopt.o getsysinfo.o gettimeofday.o getuid.o \
	ioctl.o \
	kill.o killpg.o \
	link.o listen.o lseek.o lstat.o
OBJS2=	mipsfpu.o mkdir.o mknod.o mmap.o mount.o mprotect.o\
	msgctl.o msgget.o msgrcv.o msgsnd.o munmap.o \
	nfs_biod.o nfs_getfh.o nfs_svc.o \
	open.o \
	pipe.o plock.o profil.o ptrace.o \
	quota.o \
	read.o readlink.o readv.o reboot.o recv.o recvfrom.o \
	recvmsg.o rename.o rmdir.o \
	sbrk.o select.o \
	semctl.o semget.o semop.o \
	send.o sendmsg.o sendto.o \
	setdomainname.o setgroups.o sethostid.o \
	sethostname.o setitimer.o setpgrp.o setpriority.o setquota.o \
	setregid.o setreuid.o setrlimit.o setsid.o \
	setsockopt.o setsysinfo.o settimeofday.o \
	shmat.o shmctl.o shmdt.o shmget.o shmsys.o shutdown.o \
	sigblock.o sigpause.o sigpending.o sigreturn.o sigsetmask.o \
	sigstack.o sigvec.o socket.o socketpair.o startcpu.o \
	stat.o stopcpu.o swapon.o symlink.o sync.o syscall.o \
	truncate.o \
	umask.o uname.o unlink.o unmount.o ustat.o utimes.o \
	vhangup.o \
	wait.o waitpid.o wait3.o write.o writev.o

all: $(OBJS1) $(OBJS2)

Ovadvise.o:		Ovadvise.s
Ovfork.o:		Ovfork.s
_exit.o:		_exit.s
accept.o:		accept.s
access.o:		access.s
acct.o:			acct.s
adjtime.o:		adjtime.s
atomic_op.o:		atomic_op.s
audcntl.o:		audcntl.s
audgen.o:		audgen.s
bind.o:			bind.s
brk.o:			brk.s
cachectl.o:		cachectl.s
cacheflush.o:		cacheflush.s
cerror.o:		cerror.s
chdir.o:		chdir.s
chmod.o:		chmod.s
chown.o:		chown.s
chroot.o:		chroot.s
close.o:		close.s
connect.o:		connect.s
creat.o:		creat.s
dup.o:			dup.s
dup2.o:			dup2.s
execl.o:		execl.s
execle.o:		execle.s
exect.o:		exect.s
execv.o:		execv.s
execve.o:		execve.s
exportfs.o:		exportfs.s
fchmod.o:		fchmod.s
fchown.o:		fchown.s
fcntl.o:		fcntl.s
fixade.o:		fixade.s
flock.o:		flock.s
fork.o:			fork.s
fp_sigintr.o:		fp_sigintr.s
fstat.o:		fstat.s
fsync.o:		fsync.s
ftruncate.o:		ftruncate.s
getdirentries.o:	getdirentries.s
getdomainname.o:	getdomainname.s
getdtablesize.o:	getdtablesize.s
getegid.o:		getegid.s
geteuid.o:		geteuid.s
getgid.o:		getgid.s
getgroups.o:		getgroups.s
gethostid.o:		gethostid.s
gethostname.o:		gethostname.s
getitimer.o:		getitimer.s
getmnt.o:		getmnt.s
getpagesize.o:		getpagesize.s
getpeername.o:		getpeername.s
getpgrp.o:		getpgrp.s
getpid.o:		getpid.s
getppid.o:		getppid.s
getpriority.o:		getpriority.s
getrlimit.o:		getrlimit.s
getrusage.o:		getrusage.s
getsockname.o:		getsockname.s
getsockopt.o:		getsockopt.s
getsysinfo.o:		getsysinfo.s
gettimeofday.o:		gettimeofday.s
getuid.o:		getuid.s
ioctl.o:		ioctl.s
kill.o:			kill.s
killpg.o:		killpg.s
link.o:			link.s
listen.o:		listen.s
lseek.o:		lseek.s
lstat.o:		lstat.s
m_getrusage.o:		m_getrusage.s
m_wait3.o:		m_wait3.s
mipsfpu.o:		mipsfpu.s
mkdir.o:		mkdir.s
mknod.o:		mknod.s
mmap.o:			mmap.s
mount.o:		mount.s
mprotect.o:		mprotect.s
msgctl.o:		msgctl.s
msgget.o:		msgget.s
msgrcv.o:		msgrcv.s
msgsnd.o:		msgsnd.s
munmap.o:		munmap.s
nfs_biod.o:		nfs_biod.s
nfs_getfh.o:		nfs_getfh.s
nfs_svc.o:		nfs_svc.s
open.o:			open.s
pipe.o:			pipe.s
plock.o:		plock.s
profil.o:		profil.s
ptrace.o:		ptrace.s
quota.o:		quota.s
read.o:			read.s
readlink.o:		readlink.s
readv.o:		readv.s
reboot.o:		reboot.s
recv.o:			recv.s
recvfrom.o:		recvfrom.s
recvmsg.o:		recvmsg.s
rename.o:		rename.s
rmdir.o:		rmdir.s
sbrk.o:			sbrk.s
select.o:		select.s
semctl.o:		semctl.s
semget.o:		semget.s
semop.o:		semop.s
send.o:			send.s
sendmsg.o:		sendmsg.s
sendto.o:		sendto.s
setdomainname.o:	setdomainname.s
setgroups.o:		setgroups.s
sethostid.o:		sethostid.s
sethostname.o:		sethostname.s
setitimer.o:		setitimer.s
setpgrp.o:		setpgrp.s
setpriority.o:		setpriority.s
setquota.o:		setquota.s
setregid.o:		setregid.s
setreuid.o:		setreuid.s
setrlimit.o:		setrlimit.s
setsid.o:		setsid.s
setsockopt.o:		setsockopt.s
setsysinfo.o:		setsysinfo.s
settimeofday.o:		settimeofday.s
shmat.o:		shmat.c
shmctl.o:		shmctl.c
shmdt.o:		shmdt.c
shmget.o:		shmget.c
shmsys.o:		shmsys.s
shutdown.o:		shutdown.s
sigblock.o:		sigblock.s
sigpause.o:		sigpause.s
sigpending.o:		sigpending.s
sigreturn.o:		sigreturn.s
sigsetmask.o:		sigsetmask.s
sigstack.o:		sigstack.s
sigvec.o:		sigvec.s
socket.o:		socket.s
socketpair.o:		socketpair.s
startcpu.o:		startcpu.s
stat.o:			stat.s
stopcpu.o:		stopcpu.s
swapon.o:		swapon.s
symlink.o:		symlink.s
sync.o:			sync.s
syscall.o:		syscall.s
truncate.o:		truncate.s
umask.o:		umask.s
uname.o:		uname.s
unlink.o:		unlink.s
unmount.o:		unmount.s
ustat.o:		ustat.s
utimes.o:		utimes.s
vhangup.o:		vhangup.s
wait.o:			wait.s
wait3.o:		wait3.s
waitpid.o:		waitpid.s
write.o:		write.s
writev.o:		writev.s

$(OBJS1) $(OBJS2):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*

include $(GMAKERULES)

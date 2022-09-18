#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

GENSRC= closedir.c crypt.c ctime.c dial.c execvp.c ftw.c getgrent.c \
	getopt.c \
	getpwent.c getut.c getwd.c hsearch.c \
	initgroups.c mktemp.c nlist.c opendir.c perror.c \
	putenv.c putpwent.c \
	popen.c psignal.c readdir.c scandir.c \
	system.c tell.c ttyname.c tzs.h tzset.c

GENOBJS= closedir.o crypt.o ctime.o dial.o execvp.o ftw.o getgrent.o \
	getopt.o \
	getpwent.o getut.o getwd.o hsearch.o \
	initgroups.o mktemp.o nlist.o opendir.o perror.o \
	putenv.o putpwent.o \
	popen.o psignal.o readdir.o scandir.o \
	system.o tell.o ttyname.o tzset.o

OBJS=	abort.o getlogin.o getpass.o isatty.o rand.o \
	ttyslot.o 

include ../Makelocal_$(MACHINE).mk

abort.o:		abort.c
closedir.o:		closedir.c
crypt.o:		crypt.c
ctime.o:		ctime.c tzs.h
dial.o:			dial.c
execvp.o:		execvp.c
ftw.o:			ftw.c
getgrent.o:		getgrent.c
getlogin.o:		getlogin.c
getopt.o:		getopt.c
getpass.o:		getpass.c
getpwent.o:		getpwent.c
getut.o:		getut.c
getwd.o:		getwd.c
hsearch.o:		hsearch.c
initgroups.o:		initgroups.c
isatty.o:		isatty.c
mktemp.o:		mktemp.c
nlist.o:		nlist.c
opendir.o:		opendir.c
perror.o:		perror.c
putenv.o:		putenv.c
putpwent.o:		putpwent.c
popen.o:		popen.c
psignal.o:		psignal.c
rand.o:			rand.c
readdir.o:		readdir.c
scandir.o:		scandir.c
system.o:		system.c
tell.o:			tell.c
ttyname.o:		ttyname.c
ttyslot.o:		ttyslot.c
tzset.o:		tzset.c tzs.h
getpw.o:		getpw.c

all:	getpw.o $(GENOBJS)

$(GENSRC):
	$(RM) $@
	ln -s ../../../libc/gen/$@ $@

getpw.c:
	$(RM) getpw.c
	ln -s ../../../libc/compat-4.1/getpw.c getpw.c

include $(GMAKERULES)

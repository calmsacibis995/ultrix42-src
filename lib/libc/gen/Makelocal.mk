#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

VOBJS=	a64l.o bsearch.o clock.o dial.o drand48.o ftok.o ftw.o \
	getcwd.o getopt.o getut.o hsearch.o jcsetpgrp.o l3.o l64a.o \
	lfind.o lockf.o lsearch.o memccpy.o memchr.o putenv.o  \
	putpwent.o setenv.o ssignal.o strtod.o strchr.o strcspn.o strncmp.o \
	strpbrk.o strrchr.o strspn.o strtok.o strtol.o strtoul.o tfind.o \
	tsearch.o tell.o v_regcmp.o v_regex.o

STDOBJS=_asctime.o _index.o _locale.o alarm.o assert.o calloc.o \
	closedir.o conv.o crypt.o ctime.o ctype.o ctype_.o \
	disktab.o execvp.o fstab.o \
	form_lang.o gcvt.o getenv.o getgrent.o getlogin.o getmountent.o \
	getpass.o getpwent.o getsvc.o getttyent.o getttynam.o getwd.o \
	i_asctime.o i_cnvtyp.o i_conv.o i_errno.o i_getstr.o i_init.o \
	i_ldcnv.o i_ldcol.o i_ldprp.o i_ldstr.o i_string.o \
	i_subr.o initgroups.o \
	isalnum.o isalpha.o isascii.o iscntrl.o isdigit.o isgraph.o \
	islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
	isatty.o malloc.o mblen.o mbtowc.o mbstowcs.o \
	mkfifo.o mk_lpath.o mktemp.o ndbm.o \
	opendir.o pathconf.o perror.o popen.o psignal.o qsort.o \
	raise.o random.o readdir.o regex.o remove.o reslink.o rewinddir.o \
	scandir.o seekdir.o setgid.o setegid.o \
	setrgid.o setuid.o seteuid.o setpgid.o setlocale.o \
	setruid.o sigaction.o \
	siginterrupt.o sigsetops.o siglist.o signal.o sigprocmask.o \
	sigsuspend.o sleep.o statfs.o strcoll.o strerror.o strftime.o strstr.o \
	strxfrm.o swab.o sysconf.o syslog.o system.o \
	telldir.o tsetgrp.o termios.o time.o timezone.o \
	toascii.o ttyname.o ttyslot.o tzset.o valloc.o wait2.o \
	wctomb.o wcstombs.o

FPOBJS= dtoi.o ftoi.o gtoi.o itod.o itof.o itog.o

include ../Makelocal_$(MACHINE).mk

_asctime.o:	_asctime.c
_index.o:	_index.c
_locale.o:	_locale.c
a64l.o:		a64l.c
alarm.o:	alarm.c
assert.o:	assert.c
atoi.o:		atoi.c
atol.o:		atol.c
bsearch.o:	bsearch.c
calloc.o:	calloc.c
clock.o:	clock.c
closedir.o:	closedir.c
conv.o:		conv.c
crypt.o:	crypt.c
ctime.o:	ctime.c tzs.h
ctype.o:	ctype.c
ctype_.o:	ctype_.c
dial.o:		dial.c
drand48.o:	drand48.c
dtoi.o:		dtoi.c
disktab.o:	disktab.c
execvp.o:	execvp.c
form_lang.o:	form_lang.c
fstab.o:	fstab.c
ftoi.o:		ftoi.c
ftok.o:		ftok.c
ftw.o:		ftw.c
gcvt.o:		gcvt.c
getcwd.o:	getcwd.c
getenv.o:	getenv.c
getgrent.o:	getgrent.c
getlogin.o:	getlogin.c
getmountent.o:	getmountent.c
getopt.o:	getopt.c
getpass.o:	getpass.c
getpwent.o:	getpwent.c
getsvc.o:	getsvc.c
getttyent.o:	getttyent.c
getttynam.o:	getttynam.c
getut.o:	getut.c
getwd.o:	getwd.c
gtoi.o:		gtoi.c
hsearch.o:	hsearch.c
i_asctime.o:	i_asctime.c
i_cnvtyp.o:	i_cnvtyp.c
i_conv.o:	i_conv.c
i_errno.o:	i_errno.c
i_getstr.o:	i_getstr.c
i_init.o:	i_init.c
i_ldcnv.o:	i_ldcnv.c
i_ldcol.o:	i_ldcol.c
i_ldprp.o:	i_ldprp.c
i_ldstr.o:	i_ldstr.c
i_string.o:	i_string.c
i_subr.o:	i_subr.c
initgroups.o:	initgroups.c
isalnum.o:	isalnum.c
isalpha.o:	isalpha.c
isascii.o:	isascii.c
isatty.o:	isatty.c
iscntrl.o:	iscntrl.c
isdigit.o:	isdigit.c
isgraph.o:	isgraph.c
islower.o:	islower.c
isprint.o:	isprint.c
ispunct.o:	ispunct.c
isspace.o:	isspace.c
isupper.o:	isupper.c
isxdigit.o:	isxdigit.c
itod.o:		itod.c
itof.o:		itof.c
itog.o:		itog.c
jcsetpgrp.o:	jcsetpgrp.c
l3.o:		l3.c
l64a.o:		l64a.c
lfind.o:	lfind.c
lockf.o:	lockf.c
lsearch.o:	lsearch.c
malloc.o:	malloc.c
mblen.o:	mblen.c
mbtowc.o:	mbtowc.c
mbstowcs.o:	mbstowcs.c
memccpy.o:	memccpy.c
memchr.o:	memchr.c
mkfifo.o:	mkfifo.c
mk_lpath.o:	mk_lpath.c
mktemp.o:	mktemp.c
ndbm.o:		ndbm.c
opendir.o:	opendir.c
pathconf.o:	pathconf.c
perror.o:	perror.c
popen.o:	popen.c
psignal.o:	psignal.c
putenv.o:	putenv.c
putpwent.o:	putpwent.c
qsort.o:	qsort.c
raise.o:	raise.c
random.o:	random.c
readdir.o:	readdir.c
regex.o:	regex.c
remove.o:	remove.c
reslink.o:	reslink.c
rewinddir.o:	rewinddir.c
scandir.o:	scandir.c
seekdir.o:	seekdir.c
setegid.o:	setegid.c
setenv.o:	setenv.c
seteuid.o:	seteuid.c
setgid.o:	setgid.c
setlocale.o:	setlocale.c
setpgid.o:	setpgid.c
setrgid.o:	setrgid.c
setruid.o:	setruid.c
setuid.o:	setuid.c
sigaction.o:	sigaction.c
siginterrupt.o:	siginterrupt.c
siglist.o:	siglist.c
signal.o:	signal.c
sigprocmask.o:	sigprocmask.c
sigsetops.o:	sigsetops.c
sigsuspend.o:	sigsuspend.c
sleep.o:	sleep.c
ssignal.o:	ssignal.c
statfs.o:	statfs.c
strchr.o:	strchr.c
strcoll.o:	strcoll.c
strcspn.o:	strcspn.c
strerror.o:	strerror.c
strftime.o:	strftime.c
strncmp.o:	strncmp.c
strpbrk.o:	strpbrk.c
strrchr.o:	strrchr.c
strspn.o:	strspn.c
strstr.o:	strstr.c
strtod.o:	strtod.c
strtok.o:	strtok.c
strtol.o:	strtol.c
strtoul.o:	strtoul.c
strxfrm.o:	strxfrm.c
swab.o:		swab.c
sysconf.o:	sysconf.c
syslog.o:	syslog.c
system.o:	system.c
tell.o:		tell.c
telldir.o:	telldir.c
termios.o:	termios.c
tfind.o:	tfind.c
time.o:		time.c
timezone.o:	timezone.c
toascii.o:	toascii.c
tsearch.o:	tsearch.c
tsetgrp.o:	tsetgrp.c
ttyname.o:	ttyname.c
ttyslot.o:	ttyslot.c
tzset.o:	tzset.c tzs.h
v_regcmp.o:	v_regcmp.c
v_regex.o:	v_regex.c
valloc.o:	valloc.c
wait2.o:	wait2.c
wctomb.o:	wctomb.c
wcstombs.o:	wcstombs.c

include $(GMAKERULES)

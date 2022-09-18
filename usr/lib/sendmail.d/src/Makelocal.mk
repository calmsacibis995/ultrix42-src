#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

CINCLUDES=-I../../include -DVMUNIX -DMXDOMAIN -DBSD4_3 -DOLDJEEVES
CFLAGS=	-O
REL=
LOADLIBES=-ldbm

OBJSMISC=conf.o main.o collect.o domain.o parseaddr.o alias.o \
	deliver.o savemail.o err.o readcf.o stab.o headers.o \
	recipient.o stats.o daemon.o usersmtp.o srvrsmtp.o queue.o \
	macro.o nsyslog.o util.o clock.o trace.o envelope.o \
	sysexits.o bmove.o arpadate.o convtime.o

conf.o:		conf.c
main.o:		main.c
collect.o:	collect.c
domain.o:	domain.c
parseaddr.o:	parseaddr.c
alias.o:	alias.c
deliver.o:	deliver.c
savemail.o:	savemail.c
err.o:		err.c
readcf.o:	readcf.c
stab.o:		stab.c
headers.o:	headers.c
recipient.o:	recipient.c
stats.o:	stats.c
daemon.o:	daemon.c
usersmtp.o:	usersmtp.c
srvrsmtp.o:	srvrsmtp.c
queue.o:	queue.c
macro.o:	macro.c
nsyslog.o:	nsyslog.c
util.o:		util.c
clock.o:	clock.c
trace.o:	trace.c
envelope.o:	envelope.c
sysexits.o:	sysexits.c
bmove.o:	bmove.c
arpadate.o:	arpadate.c
convtime.o:	convtime.c

include ../Makelocal_$(MACHINE).mk

all: sendmail

sendmail: $(OBJSMISC) Version.o
	$(CC) -o sendmail Version.o $(OBJSMISC) $(LOADLIBES)
	$(CHMOD) 755 sendmail

install:
	$(INSTALL) -c -s  sendmail $(DESTROOT)/usr/lib/sendmail

version: newversion $(OBJSMISC) Version.c

newversion:
	@rm -f SCCS/p.version.c version.c
	@sccs get $(REL) -e SCCS/s.version.c
	@sccs delta -ss SCCS/s.version.c
	@sccs get -t -s SCCS/s.version.c

fullversion: $(OBJS) dumpVersion Version.o

dumpVersion:
	rm -f Version.c

$(OBJSMISC): sendmail.h conf.h

include $(GMAKERULES)

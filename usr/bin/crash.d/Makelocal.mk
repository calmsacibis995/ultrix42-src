#  @(#)Makelocal.mk	2.4  ULTRIX  7/31/89

#	@(#)Makefile	2.1	(Berkeley)	12/10/85
# optional flags are: MEASURE TESTING DEBUG

include $(GMAKEVARS)

SUBDIRS=_$(MACHINE).d

$CFLAGS=-g

AOUTS=	crash

OBJS=	addr.o buf.o callout.o cmd.o cred.o file.o  main.o map.o \
	mbuf.o mem.o misc.o mount.o mscp.o network.o proc.o stats.o \
	text.o tty.o u.o rpc.o ports.o scs.o scsi.o presto.o

MACH= 	../_$(MACHINE).d/_$(MACHINE).b/machine.o \
	../_$(MACHINE).d/_$(MACHINE).b/ops.o \
	../_$(MACHINE).d/_$(MACHINE).b/symtab.o \
	../_$(MACHINE).d/_$(MACHINE).b/stacktrace.o \
	../_$(MACHINE).d/_$(MACHINE).b/getu.o \
	../_$(MACHINE).d/_$(MACHINE).b/sysvad.o 

crash: 	${OBJS} ${MACH} 

addr.o:	addr.c
buf.o:	buf.c
callout.o:	callout.c
cmd.o:	cmd.c
cred.o:	cred.c
file.o:	file.c
main.o:	main.c
map.o:	map.c
mbuf.o:	mbuf.c
mem.o:	mem.c
misc.o:	misc.c
mount.o:	mount.c
mscp.o:	mscp.c
network.o:	network.c
proc.o:	proc.c
stats.o:	stats.c
text.o:	text.c
tty.o:	tty.c
u.o:	u.c
rpc.o:	rpc.c
ports.o: ports.c
scs.o:	scs.c
scsi.o: scsi.c
presto.o:	presto.c
install: ${AOUTS}
	install -c -m 711 -g system -s crash ${DESTROOT}/usr/bin

include $(GMAKERULES)




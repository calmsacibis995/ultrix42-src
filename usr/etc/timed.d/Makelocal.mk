#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

#	@(#)Makefile	2.1	(Berkeley)	12/10/85
# optional flags are: MEASURE TESTING DEBUG

include $(GMAKEVARS)

SUBDIRS=_$(MACHINE).d

AOUTS=	timed timedc

OBJDS=	acksend.o candidate.o correct.o \
	master.o networkdelta.o readmsg.o slave.o timed.o \
	byteorder.o measure.o

OBJCS=	cmds.o cmdtab.o timedc.o \
	byteorder.o measure.o 

CKSUM=	../_$(MACHINE).d/_$(MACHINE).b/cksum.o \
	../_$(MACHINE).d/_$(MACHINE).b/in_checksum.o 

timed: 	${OBJDS} ${CKSUM} 

timedc: ${OBJCS} ${CKSUM}


acksend.o:	acksend.c
byteorder.o:	byteorder.c
candidate.o:	candidate.c
cmds.o:		cmds.c
cmdtab.o:	cmdtab.c
correct.o:	correct.c
master.o:	master.c
measure.o:	measure.c
networkdelta.o:	networkdelta.c
readmsg.o:	readmsg.c
slave.o:	slave.c
timed.o:	timed.c
timedc.o:	timedc.c

install: ${AOUTS}
	install -s timed ${DESTROOT}/usr/etc
	install -s -o root -m 4511 timedc ${DESTROOT}/usr/etc

include $(GMAKERULES)


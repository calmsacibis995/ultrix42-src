# @(#)Makelocal.mk	4.1	(ULTRIX) 7/17/90
#
#	Makefile	4.11	83/06/25
#
# make file for intermachine communications package
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				  mode 6??
#	${ADM}/aculog		ACU accounting file, owned by ${OWNER} and
#				  mode 6?? {if ACULOG defined}
# Presently supports:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	VENTEL 212+
#	VADIC 831 RS232 adaptor
#	VADIC 3451
#	GENERIC (Any reasonbly smart modem)
# (drivers are located in aculib.a)
#
# Configuration defines:
#	DF02, DF03, DN11	ACU's supported
#	  BIZ1031, BIZ1022, VENTEL, V831, V3451, GENACU
#	ACULOG			turn on tip logging of ACU use
#	PRISTINE		no phone #'s put in ACU log file
#	CONNECT 		worthless command
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
#	bufread 		Issue one read system call rather than lots
#	NOSELECT		Use tip as two process rather than 1
#
# 001 - John Williams 17-Mar-87
#	install and sccsget change
#
include $(GMAKEVARS)

ADM=	var/adm
OWNER=	uucp
GROUP=	daemon
CONFIG= -DDF02 -DDF03 -DVENTEL -DDN11 -DBIZ1031 -DBIZ1022 -DV831 -DV3451 -DDF112

CDEFINES= -DDEFBR=300 -DDEFFS=BUFSIZ -DACULOG -DFLOCK -DONDELAY -Dbufread -DGENACU
CINCLUDES= -I..
OBJS=	acu.o cmds.o cmdtab.o cu.o hunt.o \
	log.o partab.o remote.o tip.o tipout.o value.o vars.o

SPECIAL=acutab.c remcap.c uucplock.c
DRIVERS=../aculib/*.c
SOURCES=acu.c acutab.c cmds.c cmdtab.c cu.c hunt.c \
	log.c partab.c remote.c tip.c tipout.c value.c vars.c \
	${SPECIAL} ${DRIVERS}
SUBDIRS=aculib

all:	$(SUBDIRS) tip
tip:	${OBJS} acutab.o uucplock.o ../aculib/_$(MACHINE).b/aculib.a
	${LDCMD} ${OBJS} acutab.o uucplock.o \
		../aculib/_$(MACHINE).b/aculib.a

${OBJS}: tip.h

# acutab is configuration dependent, and so depends on the makefile
acutab.o: Makefile
acutab.o: ../acutab.c
	${CCCMD} ${CONFIG} ../acutab.c

uucplock.o:	../uucplock.c
	${CCCMD} ../uucplock.c

# remote.o depends on the makefile because of DEFBR and DEFFS
remote.o: Makefile

# log.o depends on the makefile because of ACULOG
log.o:	Makefile

#../aculib/_$(MACHINE).b/aculib.a: ${DRIVERS}
#	cd ../aculib; make ${MFLAGS} all

acu.o:		acu.c
cmds.o:		cmds.c
cmdtab.o:	cmdtab.c
cu.o:		cu.c
hunt.o:		hunt.c
log.o:		log.c
partab.o:	partab.c
remote.o:	remote.c
tip.o:		tip.c
tipout.o:	tipout.c
value.o:	value.c
vars.o:		vars.c
acutab.o:	acutab.c
uucplock.o:	uucplock.c

install:
	install -c -s -m 4711 -o ${OWNER} -g ${GROUP} tip ${DESTROOT}/usr/bin/tip
	rm -f ${DESTROOT}/usr/bin/cu
	ln ${DESTROOT}/usr/bin/tip ${DESTROOT}/usr/bin/cu
#	cp /dev/null ${DESTROOT}/${ADM}/aculog
#	chown ${OWNER} ${DESTROOT}/${ADM}/aculog
#	chmod 600 ${DESTROOT}/${ADM}/aculog
#	@echo "create /etc/remote and /etc/phones"

include $(GMAKERULES)

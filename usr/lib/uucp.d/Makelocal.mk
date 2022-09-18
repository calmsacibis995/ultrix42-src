# @(#)Makelocal.mk	4.1 (ULTRIX) 7/2/90
#
#	/* VAX: 	LDFLAGS=-s, IFLAGS=-s */
#	/* 11/70:	LDFLAGS=-s -n, IFLAGS=-s -i */
#	/* 11/45:	LDFLAGS=-s -n, IFLAGS=-s -i */
#	/* 11/34:	LDFLAGS=-s -n, IFLAGS=-s */
#	/* 11/23:	LDFLAGS=-s -n, IFLAGS=-s */
#	/* Concept 32:	LDFLAGS=-s -n, IFLAGS=-s -n */
#
include $(GMAKEVARS)
LDFLAGS=
IFLAGS=
#
OWNER=uucp
GROUP=daemon
#
# This version uses 4.2bsd directory reading routines.
# Set the Make variable below to reference the "new directory" routines.
# If anything changes under mkdirs, restore, or text, you need to 
#   update Makefile.mkdirs also.
#
LIBNDIR=
TIPDIR=../../../bin/tip.d
ACULIB=$(TIPDIR)/aculib/_$(MACHINE).b/aculib.a
#
########################################################################
# Wed Dec 30 21:24:31 EST 1981
# ittvax!swatt: many changes to make this easier to manage
#
# Common object files assembled into a library (uulib.a)
#
# Targets defined are:
#	all(default)	Make all UUCP commands
#	install 	Install all executables
#	save		save old executables and install new version
#	restore 	reinstall old executables
#	new		Install executables and make all necessary
#			directories
#	ndir		make and install directory sys call library
#			and include file
#	cp		Make and install executables
#	cpdirs:
#	cpfiles:
#	cpuucp		Install uucp program
#	cpuux		Install uux program
#	cpuuxqt 	Install uuxqt program
#	cpuucico	Install uucico program
#	cpuulog 	Install uulog program
#	cpuuclean	Install uuclean program
#	cpuuname	Install uuname program
#	cpuustat	Install uustat program
#	cpuusub 	Install uusub program
#	cpuupoll	Install uupoll program
#	cpuumon 	Install uumonitor program
#	cpuumkspool	Install uumkspool program
#	cpuucomp	Install uucomp program
#	cpuurespool		Install uurespool program
#	cpuucpsetup	Install uucpsetup
#	cpgetmodems	Install getmodems
#	cpaddoutsys	Install addoutsys
#	cpaddinsys	Install addinsys
#	cpmkpass	Install mkpass
#	clean		Remove all .o files
#	clobber 	Remove all files and executables
#	get:
#	cmp:		all
#	uucp		Make uucp program
#	uux		Make uux program
#	uuxqt		Make uuxqt program
#	uucico		Make uucico program
#	uulog		Make uulog program
#	uuclean 	Make uuclean program
#	uuname		Make uuname program
#	uustat		Make uustat program
#	uusub		Make uusub program
#	uupoll		Make uupoll program
#	uucomp		Make uucomp program
#	uurespool		Make uurespool program
#	uumon		Make uumonitor program
#	uumkspool	Make uumkspool program
#	mkpass		Make mkpass program
#	mkdirs		Create necessary directories
#	lint		Run lint on everything
#	lintuucp	Run lint on uucp sources
#	lintuucico	Run lint on uucico sources
#	lintuux 	Run lint on uux sources
#	lintuuxqt	Run lint on uuxqt sources
#	lintuulog	Run lint on uulog sources
#	lintuuclean	Run lint on uuclean sources
#	lintuuname	Run lint on uuname sources
#	lintuustat	Run lint on uustat sources
#	lintuusub	Run lint on uusub sources
#	lintuupoll	Run lint on uupoll sources
#	tar		Make exportable 'tar' archive of all
#			necessary sources
#	text		Copy ascii files of DISTRIB to
#			distribution file
########################################################################
#-DNEWLDISC
# CONDEVS contains devices used by uucico
CONDEVS=-DDF0 -DHAYES -DHAYSTONE -DVENTEL -DHAYESQ -DVADIC -DDF112 -DGENERIC
CFLAGS=-O -DULTRIX -DUUSTAT -DGETHOST $(CONDEVS) -DFLOCK -DONDELAY -DNEWLDISC -I..

# Files to export with tar
SOURCES=*.c
HEADRS= *.h pk.p
METOO=	[mM]akefile *INSTALL* *CHANGE* *README* *TODO*
TFILES= $(METOO) $(HEADRS) $(SOURCES)

INSDIR=${DESTROOT}/usr/var/uucp
BIN=${DESTROOT}/usr/bin
MYPATH=
SPOOLBASE=${DESTROOT}/usr/var/spool
PUBDIR=${DESTROOT}/usr/var/spool/uucppublic
SPOOL=${DESTROOT}/usr/var/spool/uucp
XQTDIR=${DESTROOT}/usr/var/spool/uucp/.XQTDIR
OLDLOG=${DESTROOT}/usr/var/uucp/.OLD
DISTRIB= L-devices L-dialcodes L.cmds L.sys LIST.DAY \
	LIST.HOUR LIST.LONGHALL LIST.NOON LIST.NIGHT L_stat \
	README USERFILE uucp.day uucp.hour uucp.longhall \
	uucp.night uucp.noon uucp.week

PKON=pkon.o
IOCTL=
LINTOP=-hbau
COMMANDS=uucp uux uuxqt uucico uulog uuclean uuname uustat uusub uupoll uumon uucomp uumkspool uurespool uuencode uudecode uusend mkpass
COMMLIB=uulib.a
# Common object files
COMMON =anlwrk.o anyread.o assert.o cfgets.o chkpth.o \
	cpmv.o expfile.o gename.o getargs.o getopt.o getprm.o \
	getpwinfo.o gio.o fio.o gnamef.o gnxseq.o gwd.o imsg.o \
	index.o lastpart.o logent.o mailst.o pk0.o pk1.o prefix.o \
	sdmail.o setline.o shio.o subdir.o sysacct.o systat.o ub_sst.o ulockf.o \
	us_crs.o us_open.o us_rrs.o us_sst.o uucpname.o \
	versys.o xqt.o uucpdefs.o
#
# Should steal "make depend" from kernel makefile
#
# Object files which depend on "pk.h"
#
PKDEP=	gio.o fio.o pk0.o pk1.o
#
# Object files which depend on "uucp.h"
#
UUCPDEP=anlwrk.o anyread.o assert.o chkpth.o cico.o cntrl.o conn.o \
	condevs.o condefs.o prefix.o \
	cpmv.o dialout.o expfile.o gename.o getpwinfo.o gio.o fio.o gnamef.o \
	gnsys.o gnxseq.o gwd.o imsg.o ioctl.o logent.o mailst.o \
	sdmail.o setline.o shio.o subdir.o systat.o ub_sst.o ulockf.o us_crs.o \
	us_open.o us_rrs.o us_sst.o uuclean.o uucp.o uucpdefs.o \
	uucpname.o uulog.o uuname.o uustat.o uusub.o uupoll.o uux.o uuxqt.o \
	versys.o xqt.o
#
# Object files which depend on "uust.h"
#
UUSTDEP=anlwrk.o cico.o cntrl.o us_crs.o us_open.o us_rrs.o us_sst.o \
	uucp.o uustat.o
#
# Ojbect files which depend on "uusub.h"
UUSUBDEP=cico.o ub_sst.o uusub.o

#
LFILES=assert.c cpmv.c expfile.c gename.c getpwinfo.c index.c lastpart.c \
	prefix.c shio.c ulockf.c xqt.c
OUUCP=uucp.o $(COMMLIB)
LUUCP=uucpdefs.c uucp.c gwd.c chkpth.c getargs.c logent.c uucpname.c\
	versys.c us_crs.c us_open.c
OUUX=uux.o $(COMMLIB)
LUUX=uucpdefs.c uux.c gwd.c anyread.c chkpth.c getargs.c getprm.c\
	logent.c uucpname.c versys.c
OUUXQT=uuxqt.o gnsys.o $(COMMLIB)
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c gnamef.c logent.c uucpname.c \
	chkpth.c getargs.c anyread.c gnsys.c
OUUCICO=cico.o cntrl.o conn.o condevs.o condefs.o dialout.o gnsys.o $(COMMLIB)
LUUCICO=uucpdefs.c cico.c cntrl.c conn.c dialout.c pk0.c pk1.c gio.c fio.c anyread.c \
	condevs.c condefs.o \
	anlwrk.c chkpth.c getargs.c gnamef.c gnsys.c gnxseq.c \
	imsg.c logent.c sysacct.c systat.c \
	mailst.c uucpname.c us_rrs.c us_sst.c us_open.c ub_sst.c setline.c
OUULOG=uulog.o $(COMMLIB)
LUULOG=uucpdefs.c uulog.c prefix.c xqt.c ulockf.c gnamef.c assert.c
OUUCLEAN=uuclean.o $(COMMLIB)
LUUCLEAN=uucpdefs.c uuclean.c gnamef.c prefix.c mailst.c getpwinfo.c\
	 getargs.c
OUUNAME=uuname.o $(COMMLIB)
LUUNAME=uuname.c uucpname.c uucpdefs.c
OUUSTAT=uustat.o $(COMMLIB)
LUUSTAT=uucpdefs.c uustat.c gnamef.c getpwinfo.c us_open.c getopt.c \
	lastpart.c cpmv.c
OUUSUB=uusub.o $(COMMLIB)
LUUSUB=uucpdefs.c uusub.c getpwinfo.c us_open.c xqt.c getopt.c
OUUPOLL=uupoll.o $(COMMLIB)
LUUSUB=uucpdefs.c uupoll.c systat.c xqt.c
OUUMON=uumon.o
LUUMON=uumon.c
OUUCOMP=uucompact.o $(COMMLIB)
LUUCOMP=uucompact.c uucpname.c
OUUMKSPOOL=mkspool.o $(COMMLIB)
LUUMKSPOOL=mkspool.c
OUURESPOOL=uurespool.o $(COMMLIB)
LUUMKSPOOL=uurespool.c

all:	$(COMMANDS)
#we do not try to really fix up this makefile completely
#just copy the .c's into this .b directory
getdotcs:
	cp ../*.c ../_$(MACHINE).b

$(COMMANDS):	$(COMMLIB)

$(COMMLIB):	$(COMMON)
	rm -f $@
	ar cr $@ $(COMMON)
	ranlib $@

$(ACULIB):
	(cd $(TIPDIR)/aculib; make aculib.a)


tools2:		all mkdirs cp text
install:	mkdirs cp text

new:		mkdirs cpfiles

cp:	cpdirs cpuucp cpuux cpuuxqt cpuucico cpuulog cpuuclean cpuuname \
	cpuustat cpuusub cpuupoll cpuumon cpuucomp cpuumkspool cpuurespool \
	cpuuencode cpuudecode cpuusend cpruusend cpuucpsetup cpaddmodems \
	cpaddoutsys cpaddinsys cpmkpass

cpdirs:
	-/etc/chown $(OWNER) $(INSDIR)
	-chgrp $(GROUP) $(INSDIR)
	-chmod 755 $(INSDIR)

save:	all
	/etc/chown $(OWNER) $(INSDIR)
	chmod 755 $(INSDIR)
	-mv $(BIN)/uucp $(BIN)/OLDuucp
	cp uucp $(BIN)
	/etc/chown $(OWNER) $(BIN)/uucp
	chmod 4111 $(BIN)/uucp
	-mv $(BIN)/uux $(BIN)/OLDuux
	cp uux $(BIN)
	/etc/chown $(OWNER) $(BIN)/uux
	chmod 4111 $(BIN)/uux
	-mv $(BIN)/uupoll $(BIN)/OLDuupoll
	cp uupoll $(BIN)/uupoll
	/etc/chown $(OWNER) $(BIN)/uupoll
	chmod 4111 $(BIN)/uupoll
	-mv $(INSDIR)/uuxqt $(INSDIR)/OLDuuxqt
	cp uuxqt $(INSDIR)
	/etc/chown $(OWNER) $(INSDIR)/uuxqt
	chmod 4111 $(INSDIR)/uuxqt
	-mv $(INSDIR)/uucico $(INSDIR)/OLDuucico
	cp uucico $(INSDIR)
	/etc/chown $(OWNER) $(INSDIR)/uucico
	chmod 4111 $(INSDIR)/uucico
	-mv $(BIN)/uudecode $(BIN)/OLDuudecode
	cp uudecode $(BIN)
	/etc/chown $(OWNER) $(BIN)/uudecode
	chmod 111 $(BIN)/uudecode
	-mv $(BIN)/uuencode $(BIN)/OLDuuencode
	cp uuencode $(BIN)
	/etc/chown $(OWNER) $(BIN)/uuencode
	chmod 111 $(BIN)/uuencode
	-mv $(BIN)/uusend $(BIN)/OLDuusend
	cp uusend $(BIN)
	/etc/chown $(OWNER) $(BIN)/OLDuusend
	chmod 4111 $(BIN)/uusend
	rm -f ${BIN}/ruusend
	ln ${BIN}/uusend ${BIN}/ruusend
	-mv $(BIN)/uulog $(BIN)/OLDuulog
	cp uulog $(BIN)
	/etc/chown $(OWNER) $(BIN)/uulog
	chmod 4111 $(BIN)/uulog
	-mv $(INSDIR)/uuclean $(INSDIR)/OLDuuclean
	cp uuclean $(INSDIR)
	/etc/chown $(OWNER) $(INSDIR)/uuclean
	chmod 4111 $(INSDIR)/uuclean
	-mv $(BIN)/uuname $(BIN)/OLDuuname
	cp uuname $(BIN)
	/etc/chown $(OWNER) $(BIN)/uuname
	chmod 4111 $(BIN)/uuname
	-mv $(BIN)/uustat $(BIN)/OLDuustat
	cp uustat $(BIN)
	/etc/chown $(OWNER) $(BIN)/uustat
	chmod 4111 $(BIN)/uustat
	-mv $(INSDIR)/uusub $(INSDIR)/OLDuusub
	cp uusub $(INSDIR)
	chmod 100 $(INSDIR)/uusub
	/etc/chown $(OWNER) $(INSDIR)/uusub
	-mv $(INSDIR)/uumonitor $(INSDIR)/OLDuumonitor
	cp uumonitor $(INSDIR)
	chmod 04111 $(INSDIR)/uumonitor
	/etc/chown $(OWNER) $(INSDIR)/uumonitor
	-mv $(INSDIR)/uumkspool $(INSDIR)/OLDuumkspool
	cp uumkspool $(INSDIR)
	chmod 04111 $(INSDIR)/uumkspool
	/etc/chown $(OWNER) $(INSDIR)/uumkspool
	-mv $(INSDIR)/uurespool $(INSDIR)/OLDuurespool
	cp uurespool $(INSDIR)
	chmod 04111 $(INSDIR)/uurespool
	/etc/chown $(OWNER) $(INSDIR)/uurespool
	-mv $(INSDIR)/uucompact $(INSDIR)/OLDuucompact
	cp uucompact $(INSDIR)
	chmod 04111 $(INSDIR)/uucompact
	/etc/chown $(OWNER) $(INSDIR)/uucompact

restore:
	-chmod u+w $(BIN)/uucp
	-mv $(BIN)/OLDuucp $(BIN)/uucp
	/etc/chown $(OWNER) $(BIN)/uucp
	chmod 4111 $(BIN)/uucp
	-chmod u+w $(BIN)/uux
	-mv $(BIN)/OLDuux $(BIN)/uux
	/etc/chown $(OWNER) $(BIN)/uux
	chmod 4111 $(BIN)/uux
	-chmod u+w $(INSDIR)/uuxqt
	-mv $(INSDIR)/OLDuuxqt $(INSDIR)/uuxqt
	/etc/chown $(OWNER) $(INSDIR)/uuxqt
	chmod 4111 $(INSDIR)/uuxqt
	-chmod u+w $(INSDIR)/uucico
	-mv $(INSDIR)/OLDuucico $(INSDIR)/uucico
	/etc/chown $(OWNER) $(INSDIR)/uucico
	chmod 4111 $(INSDIR)/uucico
	-chmod u+w $(BIN)/uulog
	-mv $(BIN)/OLDuulog $(BIN)/uulog
	/etc/chown $(OWNER) $(BIN)/uulog
	chmod 4111 $(BIN)/uulog
	-chmod u+w $(INSDIR)/uuclean
	-mv $(INSDIR)/OLDuuclean $(INSDIR)/uuclean
	/etc/chown $(OWNER) $(INSDIR)/uuclean
	chmod 4111 $(INSDIR)/uuclean
	-chmod u+w $(BIN)/uupoll
	-mv $(BIN)/OLDuupoll $(BIN)/uupoll
	chown $(OWNER) $(BIN)/uupoll
	chmod 4111 $(BIN)/uupoll
	-chmod u+w $(BIN)/uudecode
	-mv $(BIN)/OLDuudecode $(BIN)/uudecode
	chown $(OWNER) $(BIN)/uudecode
	chmod 111 $(BIN)/uudecode
	-chmod u+w $(BIN)/uuencode
	-mv $(BIN)/OLDuuencode $(BIN)/uuencode
	chown $(OWNER) $(BIN)/uuencode
	chmod 111 $(BIN)/uuencode
	-chmod u+w $(BIN)/uusend
	-mv $(BIN)/OLDuusend $(BIN)/uusend
	chown $(OWNER) $(BIN)/uusend
	chmod 4111 $(BIN)/uusend
	rm -f $(BIN)/ruusend
	ln $(BIN)/uusend $(BIN)/ruusend
	-chmod u+w $(BIN)/uuname
	-mv $(BIN)/OLDuuname $(BIN)/uuname
	/etc/chown $(OWNER) $(BIN)/uuname
	chmod 4111 $(BIN)/uuname
	-chmod u+w $(BIN)/uustat
	-mv $(BIN)/OLDuustat $(BIN)/uustat
	/etc/chown $(OWNER) $(BIN)/uustat
	chmod 4111 $(BIN)/uustat
	-chmod u+w $(INSDIR)/uusub
	-mv $(INSDIR)/OLDuusub $(INSDIR)/uusub
	chmod 100 $(INSDIR)/uusub
	/etc/chown $(OWNER) $(INSDIR)/uusub
	-chmod u+w $(INSDIR)/uumonitor
	-mv $(INSDIR)/OLDuumonitor $(INSDIR)/uumonitor
	chmod 04111 $(INSDIR)/uumonitor
	/etc/chown $(OWNER) $(INSDIR)/uumonitor
	-chmod u+w $(INSDIR)/uumkspool
	-mv $(INSDIR)/OLDuumkspool $(INSDIR)/uumkspool
	chmod 04111 $(INSDIR)/uumkspool
	/etc/chown $(OWNER) $(INSDIR)/uumkspool
	-chmod u+w $(INSDIR)/uurespool
	-mv $(INSDIR)/OLDuurespool $(INSDIR)/uurespool
	chmod 04111 $(INSDIR)/uurespool
	/etc/chown $(OWNER) $(INSDIR)/uurespool
	-chmod u+w $(INSDIR)/uucompact
	-mv $(INSDIR)/OLDuucompact $(INSDIR)/uucompact
	chmod 04111 $(INSDIR)/uucompact
	/etc/chown $(OWNER) $(INSDIR)/uucompact


cpuucp:
	-$(RM) $(BIN)/uucp 
	-cp uucp $(BIN)
	-strip $(BIN)/uucp
	-/etc/chown $(OWNER) $(BIN)/uucp
	-chgrp $(GROUP) $(BIN)/uucp
	-chmod 6111 $(BIN)/uucp

cpuux:
	-$(RM) $(BIN)/uux 
	-cp uux $(BIN)
	-strip $(BIN)/uux
	-/etc/chown $(OWNER) $(BIN)/uux
	-chgrp $(GROUP) $(BIN)/uux
	-chmod 6111 $(BIN)/uux

cpuuxqt:
	-$(RM) $(BIN)/uuxqt 
	-cp uuxqt $(INSDIR)
	-strip $(INSDIR)/uuxqt
	-/etc/chown $(OWNER) $(INSDIR)/uuxqt
	-chgrp $(GROUP) $(INSDIR)/uuxqt
	-chmod 6111 $(INSDIR)/uuxqt

cpuucico:
	-$(RM) $(BIN)/uucico 
	-cp uucico $(INSDIR)
	-strip $(INSDIR)/uucico
	-/etc/chown $(OWNER) $(INSDIR)/uucico
	-chgrp $(GROUP) $(INSDIR)/uucico
	-chmod 6111 $(INSDIR)/uucico

cpuulog:
	-$(RM) $(BIN)/uulog 
	-cp uulog $(BIN)
	-strip $(BIN)/uulog
	-/etc/chown $(OWNER) $(BIN)/uulog
	-chgrp $(GROUP) $(BIN)/uulog
	-chmod 6111 $(BIN)/uulog

cpuuclean:
	-$(RM) $(BIN)/uuclean 
	-cp uuclean $(INSDIR)
	-strip $(INSDIR)/uuclean
	-/etc/chown $(OWNER) $(INSDIR)/uuclean
	-chgrp $(GROUP) $(INSDIR)/uuclean
	-chmod 6110 $(INSDIR)/uuclean

cpuuname:
	-$(RM) $(BIN)/uuname 
	-cp uuname $(BIN)
	-strip $(BIN)/uuname
	-/etc/chown $(OWNER) $(BIN)/uuname
	-chgrp $(GROUP) $(BIN)/uuname
	-chmod 6111 $(BIN)/uuname

cpuustat:
	-$(RM) $(BIN)/uustat 
	-cp uustat $(BIN)
	-strip $(BIN)/uustat
	-/etc/chown $(OWNER) $(BIN)/uustat
	-chgrp $(GROUP) $(BIN)/uustat
	-chmod 6111 $(BIN)/uustat

cpuusub:
	-$(RM) $(INSDIR)/uusub 
	-cp uusub $(INSDIR)
	-strip $(INSDIR)/uusub
	-/etc/chown $(OWNER) $(INSDIR)/uusub
	-chgrp $(GROUP) $(INSDIR)/uusub
	-chmod 100 $(INSDIR)/uusub

cpuumon:
	-$(RM) $(INSDIR)/uumonitor 
	-cp uumonitor $(INSDIR)
	-strip $(INSDIR)/uumonitor
	-/etc/chown $(OWNER) $(INSDIR)/uumonitor
	-chgrp $(GROUP) $(INSDIR)/uumonitor
	-chmod 04111 $(INSDIR)/uumonitor

cpuumkspool:
	-$(RM) $(INSDIR)/uumkspool 
	-cp uumkspool $(INSDIR)
	-strip $(INSDIR)/uumkspool
	-/etc/chown $(OWNER) $(INSDIR)/uumkspool
	-chgrp $(GROUP) $(INSDIR)/uumkspool
	-chmod 04111 $(INSDIR)/uumkspool

cpuucomp:
	-$(RM) $(INSDIR)/uucompact 
	-cp uucompact $(INSDIR)
	-strip $(INSDIR)/uucompact
	-/etc/chown $(OWNER) $(INSDIR)/uucompact
	-chgrp $(GROUP) $(INSDIR)/uucompact
	-chmod 04111 $(INSDIR)/uucompact

cpuurespool:
	-$(RM) $(INSDIR)/uurespool 
	-cp uurespool $(INSDIR)
	-strip $(INSDIR)/uurespool
	-/etc/chown $(OWNER) $(INSDIR)/uurespool
	-chgrp $(GROUP) $(INSDIR)/uurespool
	-chmod 04100 $(INSDIR)/uurespool

cpuupoll:
	-$(RM) $(BIN)/uupoll 
	-cp uupoll $(BIN)
	-strip $(BIN)/uupoll
	-/etc/chown $(OWNER) $(BIN)/uupoll
	-chgrp $(GROUP) $(BIN)/uupoll
	-chmod 6111 $(BIN)/uupoll
	-$(RM) $(INSDIR)/uupoll 
	-cp uupoll $(INSDIR)
	-strip $(INSDIR)/uupoll
	-/etc/chown $(OWNER) $(INSDIR)/uupoll
	-chgrp $(GROUP) $(INSDIR)/uupoll
	-chmod 6111 $(INSDIR)/uupoll

cpuusend:
	-$(RM) $(BIN)/uusend 
	-cp uusend $(BIN)
	-strip $(BIN)/uusend
	-/etc/chown $(OWNER) $(BIN)/uusend
	-chgrp $(GROUP) $(BIN)/uusend
	-chmod 6111 $(BIN)/uusend

cpruusend:
	-rm -f ${BIN}/ruusend
	-ln $(BIN)/uusend $(BIN)/ruusend

cpuudecode:
	-$(RM) $(BIN)/uudecode 
	-cp uudecode $(BIN)
	-strip $(BIN)/uudecode
	-/etc/chown $(OWNER) $(BIN)/uudecode
	-chgrp $(GROUP) $(BIN)/uudecode
	-chmod 111 $(BIN)/uudecode

cpuuencode:
	-$(RM) $(BIN)/uuencode 
	-cp uuencode $(BIN)/uuencode
	-strip $(BIN)/uuencode
	-/etc/chown $(OWNER) $(BIN)/uuencode
	-chgrp $(GROUP) $(BIN)/uuencode
	-chmod 111 $(BIN)/uuencode

cpuucpsetup:
	-$(RM) ${DESTROOT}/etc/uucpsetup ${DESTROOT}/usr/etc/uucpsetup
	-cp ../uucpsetup.sh ${DESTROOT}/usr/etc/uucpsetup
	-/etc/chown $(OWNER) ${DESTROOT}/usr/etc/uucpsetup
	-chgrp $(GROUP) ${DESTROOT}/usr/etc/uucpsetup
	-chmod 555 ${DESTROOT}/usr/etc/uucpsetup
	-ln -s ../usr/etc/uucpsetup ${DESTROOT}/etc/uucpsetup

cpaddmodems:
	-$(RM) $(INSDIR)/addmodems
	-cp ../addmodems.sh $(INSDIR)/addmodems
	-/etc/chown $(OWNER) $(INSDIR)/addmodems
	-chgrp $(GROUP) $(INSDIR)/addmodems
	-chmod 555 $(INSDIR)/addmodems

cpaddoutsys:
	-$(RM) $(INSDIR)/addoutsys
	-cp ../addoutsys.sh $(INSDIR)/addoutsys
	-/etc/chown $(OWNER) $(INSDIR)/addoutsys
	-chgrp $(GROUP) $(INSDIR)/addoutsys
	-chmod 555 $(INSDIR)/addoutsys

cpaddinsys:
	-$(RM) $(INSDIR)/addinsys
	-cp ../addinsys.sh $(INSDIR)/addinsys
	-/etc/chown $(OWNER) $(INSDIR)/addinsys
	-chgrp $(GROUP) $(INSDIR)/addinsys
	-chmod 555 $(INSDIR)/addinsys

cpmkpass:
	-$(RM) $(INSDIR)/mkpass
	-cp mkpass $(INSDIR)/mkpass
	-strip $(INSDIR)/mkpass
	-/etc/chown $(OWNER) $(INSDIR)/mkpass
	-chgrp $(GROUP) $(INSDIR)/mkpass
	-chmod 500 $(INSDIR)/mkpass

cmp:	all
	cmp uucp $(BIN)/uucp
	cmp uux $(BIN)/uux
	cmp uuxqt $(INSDIR)/uuxqt
	cmp uucico $(INSDIR)/uucico
	cmp uulog $(BIN)/uulog
	cmp uuclean $(INSDIR)/uuclean
	cmp uuname $(BIN)/uuname
	cmp uustat $(BIN)/uustat
	cmp uusub $(INSDIR)/uusub
	cmp uupoll $(BIN)/uupoll
	cmp uusend $(BIN)/uusend
	cmp uuencode $(BIN)/uuencode
	cmp uudecode $(BIN)/uudecode
	cmp uumonitor $(INSDIR)/uumonitor
	cmp uumkspool $(INSDIR)/uumkspool
	cmp uurespool $(INSDIR)/uurespool
	cmp uucompact $(INSDIR)/uucompact

ndir:	./ndir/make install

uucp:	$(OUUCP)
	$(CC) $(LDFLAGS) $(OUUCP) $(LIBNDIR) -o uucp

uux:	$(OUUX)
	$(CC) $(LDFLAGS) $(OUUX) $(LIBNDIR) -o uux

uuxqt:	$(OUUXQT)
	$(CC) $(LDFLAGS) $(OUUXQT) $(LIBNDIR) -o uuxqt

uucico: $(OUUCICO)  $(IOCTL) $(PKON) $(ACULIB)
	$(CC) $(IFLAGS) $(OUUCICO) $(IOCTL) $(PKON) $(LIBNDIR) $(ACULIB) -o uucico

uulog:	$(OUULOG)
	$(CC) $(LDFLAGS) $(OUULOG) $(LIBNDIR) -o uulog

uuclean:  $(OUUCLEAN)
	$(CC) $(LDFLAGS) $(OUUCLEAN) $(LIBNDIR) -o uuclean

uuname: $(OUUNAME)
	$(CC) $(LDFLAGS) $(OUUNAME) $(LIBNDIR) -o uuname

uustat: $(OUUSTAT)
	$(CC) $(LDFLAGS) $(OUUSTAT) $(LIBNDIR) -o uustat

mkpass: mkpass.o
	$(CC) -o mkpass mkpass.o

uusub:	$(OUUSUB)
	$(CC) $(LDFLAGS) $(OUUSUB) $(LIBNDIR) -o uusub

uumon:	uumonitor

uumonitor:	$(OUUMON)
	$(CC)  $(OUUMON)  $(LIBNDIR) -o uumonitor

uucomp:	uucompact

uucompact: $(OUUCOMP)
	$(CC)  $(OUUCOMP)  $(LIBNDIR) -o uucompact

uurespool:	$(OUURESPOOL)
	$(CC)  $(OUURESPOOL)  $(LIBNDIR) -o uurespool

uumkspool: $(OUUMKSPOOL)
	$(CC)  $(OUUMKSPOOL)  $(LIBNDIR) -o uumkspool

uupoll: $(OUUPOLL)
	$(CC) $(LDFLAGS) $(OUUPOLL) -o uupoll

uuencode: uuencode.o uulib.a
	$(CC) -o uuencode ${LDFLAGS} uuencode.o uulib.a

uudecode: uudecode.o uulib.a
	$(CC) -o uudecode ${LDFLAGS} uudecode.o uulib.a

uusend: uusend.o uulib.a
	$(CC) -o uusend ${LDFLAGS} uusend.o uulib.a

#
# Header file dependencies
#
$(PKDEP):	pk.h
$(UUCPDEP):	uucp.h
$(UUSTDEP):	uust.h
$(UUSUBDEP):	uusub.h

FRC:

mkdirs:
	@-if [ ! -d ${SPOOLBASE} ] ;\
	then \
		mkdir ${SPOOLBASE}; \
		/etc/chown root ${SPOOLBASE}; \
		chgrp system ${SPOOLBASE}; \
		chmod 0755 ${SPOOLBASE}; \
	else true; \
	fi
	@-if [ ! -d ${INSDIR} ] ;\
	then \
		mkdir ${INSDIR}; \
		/etc/chown root ${INSDIR}; \
		chgrp system ${INSDIR}; \
		chmod 0755 ${INSDIR}; \
	else true; \
	fi
	@-if [ ! -d ${SPOOL} ] ;\
	then \
		mkdir ${SPOOL}; \
		/etc/chown ${OWNER} ${SPOOL}; \
		chgrp ${GROUP} ${SPOOL}; \
		chmod 0777 ${SPOOL}; \
	else true; \
	fi
	@-if [ ! -d ${SPOOL}/sys ] ;\
	then \
		mkdir ${SPOOL}/sys; \
		/etc/chown ${OWNER} ${SPOOL}/sys; \
		chgrp ${GROUP} ${SPOOL}/sys; \
		chmod 0755 ${SPOOL}/sys; \
	else true; \
	fi
	-(./uumkspool DEFAULT; true)
	@-if [ ! -d ${SPOOL}/TM. ] ;\
	then \
		mkdir ${SPOOL}/TM.; \
		/etc/chown ${OWNER} ${SPOOL}/TM.; \
		chgrp ${GROUP} ${SPOOL}/TM.; \
		chmod 0755 ${SPOOL}/TM.; \
	else true; \
	fi
	@-if [ ! -d ${SPOOL}/STST. ] ;\
	then \
		mkdir ${SPOOL}/STST.; \
		/etc/chown ${OWNER} ${SPOOL}/STST.; \
		chgrp ${GROUP} ${SPOOL}/STST.; \
		chmod 0755 ${SPOOL}/STST.; \
	else true; \
	fi
	@-if [ ! -d ${PUBDIR} ] ;\
	then \
		mkdir ${PUBDIR}; \
		/etc/chown ${OWNER} ${PUBDIR}; \
		chgrp ${GROUP} ${PUBDIR}; \
		chmod 0777 ${PUBDIR}; \
	else true; \
	fi
	@-if [ ! -d ${XQTDIR} ] ;\
	then \
		mkdir ${XQTDIR}; \
		/etc/chown ${OWNER} ${XQTDIR}; \
		chgrp ${GROUP} ${XQTDIR}; \
		chmod 0755 ${XQTDIR}; \
	else true; \
	fi
	-rm -rf ${DESTROOT}/usr/lib/uucp
	-ln -s ../var/uucp ${DESTROOT}/usr/lib/uucp
#	-mkdir $(OLDLOG)
#	chmod 777 $(OLDLOG)
#	/etc/chown $(OWNER) $(OLDLOG)
#	chgrp $(GROUP) $(OLDLOG)

cpfiles:
	cp $(MYPATH)/L* $(MYPATH)/USERFILE ${DESTROOT}$(INSDIR)
#	cp $(MYPATH)/uudemon* ${DESTROOT}$(INSDIR)
#	chmod 755 ${DESTROOT}$(INSDIR)/uudemon*
	chmod 600 $(DESTROOT)$(INSDIR)/L* ${DESTROOT}$(INSDIR)/USERFILE
	-/etc/chown $(OWNER) ${DESTROOT}$(INSDIR)/*
	-chgrp $(GROUP) ${DESTROOT}$(INSDIR)/*

lint:	lintuucp lintuucico lintuux lintuuxqt lintuulog lintuuclean\
	lintuuname lintuustat lintuusub
lintuucp:
	lint $(LINTOP) $(LUUCP) $(LFILES)

lintuucico:
	lint $(LINTOP) $(LUUCICO) $(LFILES)

lintuux:
	lint $(LINTOP) $(LUUX) $(LFILES)

lintuuxqt:
	lint $(LINTOP) $(LUUXQT) $(LFILES)

lintuulog:
	lint $(LINTOP) $(LUULOG)

lintuuclean:
	lint $(LINTOP) $(LUUCLEAN)

lintuuname:
	lint $(LINTOP) $(LUUNAME)

lintuustat:
	lint $(LINTOP) $(LUUSTAT) $(LFILES)

lintuusub:
	lint $(LINTOP) $(LUUSUB) $(LFILES)

# Make exportable 'tar' archive package
#
tar:
	tar cbf 1 uucp.tar $(TFILES)

text:
	for i in ${DISTRIB}; do \
	install -c ../$$i ${INSDIR}/$$i; done
	install -c -m 644 -o ${OWNER} -g ${GROUP} /dev/null ${SPOOL}/ERRLOG
	install -c -m 644 -o ${OWNER} -g ${GROUP} /dev/null ${SPOOL}/LOGFILE
	install -c -m 644 -o ${OWNER} -g ${GROUP} /dev/null ${PUBDIR}/.hushlogin
	install -c -m 400 -o ${OWNER} -g ${GROUP} /dev/null ${INSDIR}/.rhosts
	install -c -m 644 -o ${OWNER} -g ${GROUP} /dev/null ${INSDIR}/INSECURE
	install -c -m 644 -o ${OWNER} -g ${GROUP} /dev/null ${INSDIR}/R_stat
	install -c ../Makefile.mkdirs ${INSDIR}/Makefile

anlwrk.o:	anlwrk.c
anyread.o:	anyread.c
assert.o:	assert.c
cfgets.o:	cfgets.c
chkpth.o:	chkpth.c
cico.o:	cico.c
cntrl.o:	cntrl.c
compact.o:	compact.c
condefs.o:	condefs.c
condevs.o:	condevs.c
conn.o:	conn.c
cpmv.o:	cpmv.c
dialout.o:	dialout.c
expfile.o:	expfile.c
fio.o:	fio.c
gename.o:	gename.c
getargs.o:	getargs.c
getopt.o:	getopt.c
getprm.o:	getprm.c
getpw.o:	getpw.c
getpwinfo.o:	getpwinfo.c
gio.o:	gio.c
gnamef.o:	gnamef.c
gnsys.o:	gnsys.c
gnxseq.o:	gnxseq.c
gwd.o:	gwd.c
imsg.o:	imsg.c
index.o:	index.c
ioctl.o:	ioctl.c
lastpart.o:	lastpart.c
logent.o:	logent.c
mailst.o:	mailst.c
mcmdialout.o:	mcmdialout.c
mkpass.o:	mkpass.c
mkspool.o:	mkspool.c
pk0.o:	pk0.c
pk1.o:	pk1.c
pkon.o:	pkon.c
prefix.o:	prefix.c
sdmail.o:	sdmail.c
setline.o:	setline.c
shio.o:	shio.c
subdir.o:	subdir.c
sysacct.o:	sysacct.c
systat.o:	systat.c
ub_sst.o:	ub_sst.c
ulockf.o:	ulockf.c
us_crs.o:	us_crs.c
us_open.o:	us_open.c
us_rrs.o:	us_rrs.c
us_sst.o:	us_sst.c
uuclean.o:	uuclean.c
uucompact.o:	uucompact.c
uucp.o:	uucp.c
uucpdefs.o:	uucpdefs.c
uucpname.o:	uucpname.c
uudecode.o:	uudecode.c
uuencode.o:	uuencode.c
uulog.o:	uulog.c
uumon.o:	uumon.c
uuname.o:	uuname.c
uupoll.o:	uupoll.c
uurespool.o:	uurespool.c
uusend.o:	uusend.c
uustat.o:	uustat.c
uusub.o:	uusub.c
uux.o:	uux.c
uuxqt.o:	uuxqt.c
versys.o:	versys.c
xqt.o:	xqt.c

include $(GMAKERULES)

.c.o:
	$(CC) $(CFLAGS) -c ../$<


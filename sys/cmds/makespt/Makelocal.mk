#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	The execution of this makefile is controlled by the Makefile in
#	the sys/cmds directory.  Currently it only executes this makefile
#	in vax mode.
#
include $(GMAKEVARS)

DESTROOT=
DESTDIR=$(DESTROOT)/usr/sys/cmds


OBJSMISC=expr.o makespt.o

CFILES= makespt.c
CFLAGS=-O -DSUBR 

all: makespt
makespt:$(OBJSMISC)
	$(LDCMD) $(OBJSMISC) $(LDLIBRARIES)
	strip makespt

makespt.o: makespt.c
expr.o: expr.y

install: inst-$(MACHINE)

inst-vax:
	-if [ ! -d ${DESTDIR}/makespt ] ;\
	then \
		mkdir ${DESTDIR}/makespt; \
		/etc/chown root ${DESTDIR}/makespt; \
		chgrp system ${DESTDIR}/makespt; \
		chmod 0755 ${DESTDIR}/makespt; \
	else true; \
	fi
	-if [ ! -d ${DESTDIR}/makespt/_$(MACHINE).b ] ;\
	then \
		mkdir ${DESTDIR}/makespt/_$(MACHINE).b; \
		/etc/chown root ${DESTDIR}/makespt/_$(MACHINE).b; \
		chgrp system ${DESTDIR}/makespt/_$(MACHINE).b; \
		chmod 0755 ${DESTDIR}/makespt/_$(MACHINE).b; \
	else true; \
	fi
	$(INSTALL) -s -c -m 755  makespt ${DESTDIR}/makespt/_$(MACHINE).b

inst-mips:
	@echo No mips makespt
	
include $(GMAKERULES)

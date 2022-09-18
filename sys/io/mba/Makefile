#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/io/mba
DIRS=vax

all:

clean:

clobber: clean
	for i in ${DIRS} ; do \
		$(ECHO) "$$i:";cd $$i;sccs clean;cd ..;done

sccsinfo:
	-@for i in ${DIRS}; do \
		$(ECHO) "$$i:";cd $$i;sccs info;cd .. ; done

sccsget:
	@-for i in ${DIRS};do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;cd ..;done

install: instdirs inst-$(MACHINE)

instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	@-for i in  ${DIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; done

inst-vax:
	@echo Installing vax mba files.
	@cd vax;tar cFf - *.h|(cd ${DESTDIR}/vax;tar xpf -; chmod ugo+r *)

inst-mips:
	@echo Installing mips mba files.
	@echo That's easy.

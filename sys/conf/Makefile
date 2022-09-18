#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/conf
TARDIRS=vax mips
#
#
# Files in the conf dir that go on the kit.
#
#
CONF-FILES= files


all:

clean:

clobber: clean
	cd ${MACHINE};sccs clean;
	-cd ${MACHINE};rm -f config

sccsinfo:
	@cd ${MACHINE};echo -n "${MACHINE}:";sccs info

sccsget:
	@-cd ${MACHINE};echo "${MACHINE}:";sccs get -s SCCS

install: instdirs inst-common inst-${MACHINE}

instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	@-for i in  ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; done

inst-common:
	@echo Installing common conf files
	@chmod ugo+r *;tar cFf - ${CONF-FILES} | ( cd $(DESTDIR);tar xpf - )

inst-vax:
	@echo Installing vax conf files.
	@tar cFf - vax |(cd ${DESTDIR};tar xpf -; chmod ugo+r *)

inst-mips:
	@echo Installing mips conf files.
	@tar cFf - mips|(cd ${DESTDIR};tar xpf -; chmod ugo+r *)

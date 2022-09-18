#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/io/scsi
TARDIRS=vax mips

all:

clean:

clobber: clean
	cd vax;sccs clean;
	cd mips;sccs clean

sccsinfo:
	@cd vax;echo -n "vax:";sccs info
	@cd mips;echo -n "mips:";sccs info

sccsget:
	@-cd vax;echo "vax:";sccs get -s SCCS
	@-cd mips;echo "mips:";sccs get -s SCCS

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
	@-for i in  ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; done

inst-vax:
	@echo Installing vax scsi files.
	@cd vax;tar cFf - *.h|(cd ${DESTDIR}/vax;tar xpf -; chmod ugo+r *)

inst-mips:
	@echo Installing mips scsi files.
	@cd mips;tar cFf - *.h|(cd ${DESTDIR}/mips;tar xpf -; chmod ugo+r *)


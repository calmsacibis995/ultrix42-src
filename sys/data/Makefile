#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/data
TARDIRS=vax mips

all:

clean:

clobber: clean
	cd vax;sccs clean;
	cd mips;sccs clean

sccsinfo:
	@sccs info
	@cd vax;echo -n "vax:";sccs info
	@cd mips;echo -n "mips:";sccs info

sccsget:
	@-cd vax;echo "vax:";sccs get -s SCCS
	@-cd mips;echo "mips:";sccs get -s SCCS

install: instdirs inst-common 

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
	@echo Installing common data files.
	@chmod ugo+r *;tar cFf - *.c | ( cd $(DESTDIR);tar xpf - )


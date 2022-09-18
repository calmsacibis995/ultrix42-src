#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/vm
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


insthdrs: instdirs inst-common inst-$(MACHINE)

install: instdirs inst-common inst-$(MACHINE)

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
	@echo Installing common vm files.
	@echo No common vm files to install
#	@chmod ugo+r *;tar cFf - *.h | ( cd $(DESTDIR);tar xpf - )

inst-vax:
	@echo Installing vax vm files.
	@cd vax;chmod ugo+r *;tar cFf - *.h | ( cd $(DESTDIR)/vax;tar xpf - )

inst-mips:
	@echo Installing mips vm files.
	@cd mips;chmod ugo+r *;tar cFf - *.h | ( cd $(DESTDIR)/mips;tar xpf - )

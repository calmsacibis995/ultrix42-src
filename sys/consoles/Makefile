#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/consoles
TARDIRS=mips
CDSRCDIRS=vax

all:

clean:
	-for i in ${CDSRCDIRS} ;do \
		if [ -d $$i/. ]; then \
			$(ECHO) "$$i:";cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done
	-for i in ${CDSRCDIRS} ${TARDIRS} ;do \
		if [ -d $$i/. ]; then \
			$(ECHO) "$$i:";cd $$i;sccs clean;cd ..;\
		fi; \
	done

clobber:
	for i in ${CDSRCDIRS} ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) clobber;cd ..;done

sccsinfo:
	-@sccs info
	-@for i in ${TARDIRS}; do \
		$(ECHO) "$$i:";cd $$i;sccs info;cd .. ; done
	-@for i in ${CDSRCDIRS}; do \
		$(ECHO) "$$i:";cd $$i;sccs info;$(MAKE) sccsinfo;cd .. ; done

sccsget:
	-@for i in ${TARDIRS}; do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;cd .. ; done
	@-for i in ${CDSRCDIRS};do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;$(MAKE) sccsget;cd ..;done


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
	@-for i in  ${CDSRCDIRS} ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; done

inst-common:
	@echo Installing common console files.
	@echo No common console files
#	@chmod ugo+r *;tar cFf - *.c | ( cd $(DESTDIR);tar xpf - )

inst-vax:
	@echo Installing vax console files.
	@cd vax;$(MAKE) DESTROOT=${DESTROOT} install

inst-mips:
	@echo No mips console files

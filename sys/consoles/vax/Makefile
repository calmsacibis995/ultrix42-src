#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys
TARDIRS=730cons 750cons 780cons 8200cons 8600cons

all:

clean:
	-for i in ${TARDIRS} ;do \
		if [ -d $$i/. ]; then \
			$(ECHO) "$$i:";cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done

clobber:
	for i in ${TARDIRS} ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) clobber;cd ..;done

sccsinfo:
	-@for i in ${TARDIRS}; do \
		$(ECHO) "$$i:";cd $$i;sccs info;cd .. ; done

sccsget:
	@-for i in ${TARDIRS};do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;cd ..;done


install: instdirs inst-730cons inst-750cons inst-780cons inst-8200cons \
		inst-8600cons

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

inst-750cons:
	@echo Installing 750cons files.
	@cd 750cons;make ${MFLAGS} DESTROOT=${DESTROOT} install

inst-8200cons:
	@echo Installing 8200cons files.
	@cd 8200cons;make ${MFLAGS} DESTROOT=${DESTROOT} install

inst-780cons:
	@echo Installing 780cons files.
	@cd 780cons;make ${MFLAGS} DESTROOT=${DESTROOT} install

inst-8600cons:
	@echo Installing 8600cons
	@cd 8600cons;make ${MFLAGS} DESTROOT=${DESTROOT} install

inst-730cons: 
	@echo Installing 730cons
	@cd 730cons;make ${MFLAGS} DESTROOT=${DESTROOT} install



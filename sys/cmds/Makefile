#
#	7/2/90	(ULTRIX)	@(#)Makefile	4.1
#
# Normal sequence of events is: clean clobber sccsget all install
#


DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/cmds
include Makelocal_$(MACHINE).mk

first: all

all: 

	-for i in ${TARDIRS} ; do \
		if [ -d $$i/. ]; then \
			$(ECHO) "$$i";cd $$i;$(MAKE) all;cd ..;\
		fi;\
	done
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


install:  instdirs inst-config inst-makespt

instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	rm -f ${DESTROOT}/sys
	cd ${DESTROOT}; ln -s usr/sys ${DESTROOT}/sys
	-for i in ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; \
	done

inst-config:
	echo Installing config command
	cd config;$(MAKE) DESTROOT=${DESTROOT} install

inst-makespt:
	echo Installing makespt command
	cd makespt;$(MAKE) DESTROOT=${DESTROOT} install

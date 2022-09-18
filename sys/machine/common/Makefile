#
#	@(#)Makefile	4.2 (ULTRIX) 10/11/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/machine/${MACHINE}

CLOBBERDIR=
MCLEANDIRS=
ALLDIRS=
TARDIRS=
#
#
# Files that go on the kit.
#
#
FILES=cpuconf.h cpuconf.c conf.c



all:

clean:
	-for i in ${MCLEANDIRS} ;do \
		if [ -d $$i/. ]; then \
			cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done

clobber: clean
	for i in ${CLOBBERDIR}; do \
		cd $$i;sccs clean;cd ..;done
	rm -f [abdf-hj-ln-z]* [ceim]*.*


sccsinfo:
	@sccs info
	@for i in ${ALLDIRS} ; do cd $$i;echo -n "$$i:";sccs info;cd .. ; done

sccsget:
	@-sccs get -s SCCS
	@-for i in ${ALLDIRS};do cd $$i;echo "$$i";sccs get -s SCCS;cd ..;done


install:
	@echo Making /sys/machine/common directory
	mkdir ${DESTROOT}/sys/machine/common
	@echo Installing ${FILES}
	tar cFf - ${FILES}| (cd ${DESTROOT}/sys/machine/common; tar xpf -; \
	chmod ugo+r ${FILES}; )
	@echo Making link to cpuconf.h
	ln -s /sys/machine/common/cpuconf.h ${DESTROOT}/sys/h/cpuconf.h
	ln -s /sys/machine/common/cpuconf.h ${DESTDIR}/cpuconf.h


instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	@rm -f ${DESTROOT}/sys
	@cd ${DESTROOT}; ln -s usr/sys ${DESTROOT}/sys
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

inst-inline:


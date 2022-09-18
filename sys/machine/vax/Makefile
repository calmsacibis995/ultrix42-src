#
#  @(#)Makefile	4.4	(ULTRIX)	1/21/91
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/machine/vax

CLOBBERDIR= ./emul ./inline
MCLEANDIRS= ./inline
ALLDIRS= ${CLOBBERDIR}
TARDIRS= emul
#
#
# Vax files that go on the kit.
#
#
VAX-FILES=mtpr.h pte.h trap.h b_params.h nexus.h debug.h \
    reg.h vmparam.h clock.h flp.h param.h rpb.h frame.h pcb.h rsp.h \
    cons.h mem.h pcb.m scb.h cpu.h psl.h symbols.raw symbols.sort \
    spt.s lock.s ioa.h crl.h rpb.s scb.s swap.c ka8200.h ka8800.h \
    ka60.h ka650.h ka6200.h cvax.h vectors.h ka9000.h



all: first

clean:
	-for i in ${MCLEANDIRS} ;do \
		if [ -d $$i/. ]; then \
			cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done

clobber: clean rmlinks
	for i in ${CLOBBERDIR}; do \
		cd $$i;sccs clean;cd ..;done
	rm -f [abdf-hj-ln-z]* [ceim]*.*


sccsinfo:
	@sccs info
	@for i in ${ALLDIRS} ; do cd $$i;echo -n "$$i:";sccs info;cd .. ; done

sccsget:
	@-sccs get -s SCCS
	@-for i in ${ALLDIRS};do cd $$i;echo "$$i";sccs get -s SCCS;cd ..;done


insthdrs: instdirs inst-inline inst-vax

install: instdirs inst-inline inst-vax

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

first: 
	cd inline; $(MAKE)

inst-vax:
	@echo Installing vax files.
	tar cFf - ${VAX-FILES}|(cd ${DESTDIR};tar xpf -; chmod ugo+r * ;)

inst-inline:
	@echo Installing machine/vax/inline.
	cd inline; $(MAKE) DESTROOT=${DESTROOT} install


rmlinks:
	rm -rf io
	rm -rf net
	rm -rf fs
	rm -rf vm
	rm -rf data
	rm -rf sas
	rm -rf sys
	rm -rf conf
	rm -rf dist
	rm -rf h

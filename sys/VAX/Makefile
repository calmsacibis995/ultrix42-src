#
#	@(#)Makefile	4.1	(ULTRIX)	7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

SAS_KERNELS= SAS.net SAS.gen SAS.rx01 SAS.tu58
MAKEDIRS= BINARY GENERIC ${SAS_KERNELS}
DESTROOT=/Ultrix
DESTDIR=${DESTROOT}/usr/sys/VAX
CONFDIR=../conf/vax
CONFFILES=$(CONFDIR)/files $(CONFDIR)/files.vax

all: makes

tags: 
	/usr/ucb/ctags -w `cat $(CONFFILES) | awk '{printf("../%s\n",$$1)}'` 

clean:
	-for i in ${MAKEDIRS} ;do \
		if [ -d $$i/. ]; then \
			cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done;exit 0

clobber: rmlinks
	-rm -fr ${MAKEDIRS};exit 0

sccsinfo:
	sccs info

sccsget: makelinks

first:
	-for i in ${MAKEDIRS} ;do \
		rm -fr $$i ; \
		if [ ! -d $$i ] ;\
		then \
			mkdir $$i; \
			/etc/chown root $$i; \
			chgrp system $$i; \
			chmod 0755 $$i; \
		else true; \
		fi \
	done;
	cd $(CONFDIR);./config -s BINARY
	-for i in GENERIC ${SAS_KERNELS} ; do \
		(cd $(CONFDIR);./config $$i) ; \
	done
	-for i in ${MAKEDIRS} ;do \
		(cd $$i;make depend); \
	done

BINARY:
	-rm -fr $@
	-if [ ! -d $@ ] ;\
	then \
		mkdir $@; \
		/etc/chown root $@; \
		chgrp system $@; \
		chmod 0755 $@; \
	else true; \
	fi
	cd $(CONFDIR);./config -s $@
	cd $@;make depend

GENERIC SAS.net SAS.gen SAS.rx01 SAS.tu58: BINARY
	-rm -fr $@
	-if [ ! -d $@ ] ;\
	then \
		mkdir $@; \
		/etc/chown root $@; \
		chgrp system $@; \
		chmod 0755 $@; \
	else true; \
	fi
	cd $(CONFDIR);./config $@
	cd $@;make depend

insthdrs: inst-hdr-links

install: instdirs inst-BINARY inst-GENERIC inst-SAS inst-links

inst-links:
	install -c -m 644 Makefile $(DESTROOT)/usr/sys/VAX
	-cd $(DESTROOT)/usr/sys/VAX;make makelinks
 
inst-hdr-links:
	install -c -m 644 Makefile $(DESTROOT)/usr/sys/VAX
	-cd $(DESTROOT)/usr/sys/VAX;make machlinks
 
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
	@-for i in BINARY ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; \
	done

makes: ${MAKEDIRS}
	for i in ${MAKEDIRS}; do  cd $$i;make all; cd ..  ; done

inst-BINARY: 
	@echo 'installing BINARIES for '${MACHINE}
	@-for i in ${MACHINE}; do \
		if [ ! -d ${DESTDIR}/BINARY ] ;\
		then \
			mkdir ${DESTDIR}/BINARY; \
			/etc/chown root ${DESTDIR}/BINARY; \
			chgrp system ${DESTDIR}/BINARY; \
			chmod 0755 ${DESTDIR}/BINARY; \
		else true; \
		fi; \
		(cd BINARY;tar cf - . | ( cd ${DESTDIR}/BINARY;tar xpf - \
		; rm -f locore.c .[a-zA-Z]* log; chmod ugo+r *)); \
	done;exit 0

inst-GENERIC: 
	@echo Installing Generic system
	(cd GENERIC; install -c -m 755 vmunix ${DESTROOT}/genvmunix)
	-[ -d $(DESTROOT)/usr/diskless ] || \
		mkdir $(DESTROOT)/usr/diskless
	(cd GENERIC; install -c -m 755 dlvmunix ${DESTROOT}/usr/diskless/genvmunix)
	(cd ${DESTROOT};rm -f vmunix;ln genvmunix vmunix)

inst-SAS: 
	@echo 'installing standalone kernels'
	@-for i in net gen rx01 tu58; do \
	     if [ ! -d ${DESTDIR}/SAS.$$i ] ;\
	     then \
		     mkdir ${DESTDIR}/SAS.$$i; \
		     /etc/chown root ${DESTDIR}/SAS.$$i; \
		     chgrp system ${DESTDIR}/SAS.$$i; \
		     chmod 0755 ${DESTDIR}/SAS.$$i; \
	     else true; \
	     fi; \
	     (cd SAS.$$i;install -c -m 755 vmunix ${DESTDIR}/SAS.$$i/vmunix); \
	done;exit 0

machlinks:
	ln -s ../machine/vax machine
	mkdir ../machine/vax/io
	mkdir ../machine/vax/net
	mkdir ../machine/vax/fs
	ln -s ../../vm/vax ../machine/vax/vm
	ln -s ../../data/vax ../machine/vax/data
	ln -s ../../sas/vax ../machine/vax/sas
	ln -s ../../sys/vax ../machine/vax/sys
	ln -s ../../conf/vax ../machine/vax/conf
	ln -s ../../dist/vax ../machine/vax/dist
	ln -s ../../h/vax ../machine/vax/h
	for i in fs net io; do\
	for j in `(cd ..; ls -d $$i/[a-z0-9]*) | egrep -v SCCS\|machine`; do\
	ln -s ../../../$$j/vax ../machine/vax/$$j; done; done;

makelinks: machlinks
	ln -s ../files ../conf/vax/files
	ln -s ../../cmds/config/_vax.b/config ../conf/vax/config

rmlinks:
	rm -f [a-z]* [0-9]*
	rm -f ../conf/vax/files
	rm -f ../conf/vax/config
	rm -rf ../machine/vax/io
	rm -rf ../machine/vax/net
	rm -rf ../machine/vax/fs
	rm -rf ../machine/vax/vm
	rm -rf ../machine/vax/data
	rm -rf ../machine/vax/sas
	rm -rf ../machine/vax/sys
	rm -rf ../machine/vax/conf
	rm -rf ../machine/vax/dist
	rm -rf ../machine/vax/h

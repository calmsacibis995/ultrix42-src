#
#	@(#)Makefile	4.1	(ULTRIX)	7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

MAKEDIRS=BINARY GENERIC SAS
DESTROOT=/Ultrix
DESTDIR=${DESTROOT}/usr/sys/MIPS
CONFDIR=../conf/mips
CONFFILES=$(CONFDIR)/files $(CONFDIR)/files.mips

all: makes

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
	-for i in GENERIC SAS ; do \
		(cd $(CONFDIR);./config $$i) ; \
	done
	-for i in ${MAKEDIRS} ;do \
		(cd $$i;make depend); \
	done

tags: 
#	/usr/ucb/ctags -w `cat $(CONFFILES) | awk '{printf("../%s\n",$$1)}'` 
	cat $(CONFFILES) | awk '{printf("../%s\n",$$1)}' > tag_tmp
	# split list into files with 100 lines each (else argv exceeds 10240)
	-csplit -k -ftag_tmp tag_tmp 100 {90}
	cp /dev/null tags
	-(for i in tag_tmp?? ;do \
		/usr/ucb/ctags -w -a `cat $$i`;\
	done)
	rm -f tag_tmp*

clean:
	-for i in ${MAKEDIRS} ;do \
		if [ -d $$i/. ]; then \
			cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done

clobber: rmlinks
	-rm -fr ${MAKEDIRS}

sccsinfo:
	sccs info

sccsget: makelinks

BINARY: FRC
	-rm -fr $@
	-if [ ! -d $@ ] ;\
	then \
		mkdir $@; \
		chmod 0755 $@; \
	else true; \
	fi
	(cd $(CONFDIR);./config -s $@)
	(cd $@;make depend)

GENERIC SAS: BINARY
	-rm -fr $@
	-if [ ! -d $@ ] ;\
	then \
		mkdir $@; \
		/etc/chown root $@; \
		chgrp system $@; \
		chmod 0755 $@; \
	else true; \
	fi
	(cd $(CONFDIR);./config $@)
	(cd $@;make depend)

FRC:

insthdrs: inst-hdr-links

install: instdirs inst-BINARY inst-GENERIC inst-SAS inst-links

inst-links:
	install -c -m 644 Makefile $(DESTROOT)/usr/sys/MIPS
	-cd $(DESTROOT)/usr/sys/MIPS;make makelinks

inst-hdr-links:
	install -c -m 644 Makefile $(DESTROOT)/usr/sys/MIPS
	-cd $(DESTROOT)/usr/sys/MIPS;make machlinks
 
instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	-if [ ! -d ${DESTDIR}/BINARY ] ;\
	then \
		mkdir ${DESTDIR}/BINARY; \
		/etc/chown root ${DESTDIR}/BINARY; \
		chgrp system ${DESTDIR}/BINARY; \
		chmod 0755 ${DESTDIR}/BINARY; \
	else true; \
	fi
	-if [ ! -d ${DESTDIR}/SAS ] ;\
	then \
		mkdir ${DESTDIR}/SAS; \
		/etc/chown root ${DESTDIR}/SAS; \
		chgrp system ${DESTDIR}/SAS; \
		chmod 0755 ${DESTDIR}/SAS; \
	else true; \
	fi
	rm -f ${DESTROOT}/sys
	ln -s usr/sys ${DESTROOT}

makes: ${MAKEDIRS}
	for i in ${MAKEDIRS}; do \
		(cd $$i;$(MAKE) all); done

inst-BINARY: 
	@echo Installing BINARIES for ${MACHINE}
	-if [ ! -d ${DESTDIR}/BINARY ] ;\
	then \
		mkdir ${DESTDIR}/BINARY; \
		/etc/chown root ${DESTDIR}/BINARY; \
		chgrp system ${DESTDIR}/BINARY; \
		chmod 0755 ${DESTDIR}/BINARY; \
	else true; \
	fi
#	((cd BINARY;tar cf - *.o makefile) | (cd ${DESTDIR}/BINARY;tar xpf -))
# copy header files as well as the .o's
	(cd BINARY;tar cf - . | ( cd ${DESTDIR}/BINARY;tar xpf - \
	; rm -f locore.c tags errs makefile.bak .[a-zA-Z]* log; chmod ugo+r *));


inst-GENERIC:
	@echo Installing GENERIC
	(cd GENERIC; install -c -m 755 vmunix ${DESTROOT}/genvmunix)
	-[ -d $(DESTROOT)/usr/diskless ] || \
		mkdir $(DESTROOT)/usr/diskless
	(cd GENERIC; install -c -m 755 vmunix ${DESTROOT}/usr/diskless/genvmunix)
	(cd ${DESTROOT};rm -f vmunix;ln genvmunix vmunix)

inst-SAS: 
	@echo Installing SAS
	-if [ ! -d ${DESTDIR}/SAS ] ;\
	then \
		mkdir ${DESTDIR}/SAS; \
		/etc/chown root ${DESTDIR}/SAS; \
		chgrp system ${DESTDIR}/SAS; \
		chmod 0755 ${DESTDIR}/SAS; \
	else true; \
	fi
	(cd SAS;install -c -m 755 vmunix ${DESTDIR}/SAS/vmunix)

machlinks:
	ln -s ../machine/mips machine
	mkdir ../machine/mips/io
	mkdir ../machine/mips/net
	mkdir ../machine/mips/fs
	ln -s ../../vm/mips ../machine/mips/vm
	ln -s ../../data/mips ../machine/mips/data
	ln -s ../../sas/mips ../machine/mips/sas
	ln -s ../../sys/mips ../machine/mips/sys
	ln -s ../../conf/mips ../machine/mips/conf
	ln -s ../../dist/mips ../machine/mips/dist
	ln -s ../../h/mips ../machine/mips/h
	for i in fs net io; do\
	for j in `(cd ..; ls -d $$i/[a-z0-9]*) | egrep -v SCCS\|machine`; do\
	ln -s ../../../$$j/mips ../machine/mips/$$j; done; done;

makelinks: machlinks
	ln -s ../files ../conf/mips/files
	ln -s ../../cmds/config/_mips.b/config ../conf/mips/config

rmlinks:
	rm -f [a-z]* [0-9]*
	rm -f ../conf/mips/files
	rm -f ../conf/mips/config
	rm -rf ../machine/mips/io
	rm -rf ../machine/mips/net
	rm -rf ../machine/mips/fs
	rm -rf ../machine/mips/vm
	rm -rf ../machine/mips/data
	rm -rf ../machine/mips/sas
	rm -rf ../machine/mips/sys
	rm -rf ../machine/mips/conf
	rm -rf ../machine/mips/dist
	rm -rf ../machine/mips/h

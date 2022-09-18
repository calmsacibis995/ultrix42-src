#
#	@(#)Makefile	4.2 (ULTRIX)11/9/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

CLOBBERDIR= gfs ufs nfs specfs cdfs

TARDIRS= gfs ufs nfs specfs cdfs

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/fs

all:

clean:

clobber: clean
	for i in ${CLOBBERDIR}; do \
	cd $$i;sccs clean;cd ..;done

sccsinfo:
	@for i in ${TARDIRS} ; do cd $$i;echo -n "$$i:";sccs info;cd .. ; done

sccsget:
	@-for i in ${TARDIRS};do cd $$i;echo "$$i";sccs get -s SCCS;cd ..;done

install: all instdirs inst-gfs inst-ufs inst-nfs inst-specfs inst-cdfs

instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	-for i in  ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; \
	done

inst-gfs:
	@echo Gfs, thats easy too!

inst-ufs:
	@echo Installing ufs files.
	@cd ufs;tar cFf - *.h|(cd ${DESTDIR}/ufs;tar xpf -; chmod ugo+r *)

inst-nfs:
	@echo Installing nfs files.
	@cd nfs;tar cFf - *.h|(cd ${DESTDIR}/nfs;tar xpf -; chmod ugo+r *)

inst-specfs:
	@echo Specfs, thats easy too!

inst-cdfs:
	@echo Installing cdfs files.
	@cd cdfs;tar cFf - *.h|(cd ${DESTDIR}/cdfs;tar xpf -; chmod ugo+r *)

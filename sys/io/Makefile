#
#	@(#)Makefile	4.3	(ULTRIX)	8/16/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

CLOBBERDIR=bi netif mba uba ci gvp sysap scs xmi scsi msi tc vme ws

TARDIRS=bi netif uba ci gvp sysap scs xmi msi tc vme ws

CDSRCDIRS= mba scsi

ALLDIRS = $(TARDIRS) $(CDSRCDIRS)
DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/io
#
#
# Files in the io/bi dir that go on the kit.
#
#
BI-FILES= bireg.h buareg.h bdareg.h dmbreg.h bvp_ni.h nireg.h bvpsysap.h \
    bvpscs.h  bvpport.h bvpppd.h bvpreg.h
#
#
# Files in the io/xmi dir that go on the kit.
#
#
XMI-FILES= xbireg.h xmareg.h xmireg.h
#

all: 

clean:

clobber: clean
	for i in $(CDSRCDIRS) ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) clobber;cd ..;done
	for i in ${CLOBBERDIR}; do \
		cd $$i;sccs clean;cd ..;done


sccsinfo:
	@for i in ${ALLDIRS} ; do cd $$i;echo -n "$$i:";sccs info;cd .. ; done
	-@for i in $(CDSRCDIRS) ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) sccsinfo;cd .. ; done

sccsget: 
	@-for i in ${ALLDIRS};do cd $$i;echo "$$i";sccs get -s SCCS;cd ..;done
	@-for i in $(CDSRCDIRS);do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) sccsget;cd ..;done


install: all instdirs inst-bi inst-netif inst-mba inst-uba inst-sysap \
	inst-scs inst-gvp inst-ci inst-msi inst-xmi inst-scsi \
	inst-tc inst-vme inst-ws #inst-xos 

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
inst-bi:
	@echo Installing bi files.
	cd bi;tar cFf - ${BI-FILES}|(cd ${DESTDIR}/bi;tar xpf -; chmod ugo+r *)

inst-xmi:
	@echo Installing xmi files.
	cd xmi;tar cFf - ${XMI-FILES}|(cd ${DESTDIR}/xmi;tar xpf -; chmod ugo+r *)

inst-ci:
	@echo Installing ci files.
	@cd ci;tar cFf - *.h|(cd ${DESTDIR}/ci;tar xpf -; chmod ugo+r *)

inst-msi:
	@echo Installing msi files.
	@cd msi;tar cFf - *.h|(cd ${DESTDIR}/msi;tar xpf -; chmod ugo+r *)

inst-netif:
	@echo Installing netif files.
	@cd netif;tar cFf - *.h|(cd ${DESTDIR}/netif;tar xpf -; chmod ugo+r *)

inst-mba:
	@echo Installing mba files.
	@cd mba; $(MAKE) DESTROOT=$(DESTROOT) install

inst-scsi:
	@echo Installing scsi files.
	@cd scsi; $(MAKE) DESTROOT=$(DESTROOT) install

inst-gvp:
	@echo Installing gvp files.
	@cd gvp;tar cFf - *.h|(cd ${DESTDIR}/gvp;tar xpf -; chmod ugo+r *)

inst-scs:
	@echo Installing scs files.
	@cd scs;tar cFf - *.h|(cd ${DESTDIR}/scs;tar xpf -; chmod ugo+r *)

inst-sysap:
	@echo Installing sysap files.
	@cd sysap;tar cFf - *.h|(cd ${DESTDIR}/sysap;tar xpf -; chmod ugo+r *)

inst-uba:
	@echo Installing uba files.
	@cd uba;tar cFf - *.h sp.c|(cd ${DESTDIR}/uba;tar xpf -; chmod ugo+r *)

inst-tc:
	@echo Installing tc files.
	@cd tc;tar cFf - *.h |(cd ${DESTDIR}/tc;tar xpf -; chmod ugo+r *)

inst-ws:
	@echo Installing ws files.
	@cd ws;tar cFf - *.h |(cd ${DESTDIR}/ws;tar xpf -; chmod ugo+r *)

inst-vme:
	@echo Installing vme files.
	@cd vme;tar cFf - *.h |(cd ${DESTDIR}/vme;tar xpf -; chmod ugo+r *)

#
#	10/12/90	(ULTRIX)	@(#)Makefile	4.4
#
# Normal sequence of events is: clean clobber sccsget all install
#


ECHO=echo -n

BINSRCDIRS= ./machine

BINDIRS= ./$(CAPMACHINE)
MACHDIRS = ./VAX ./MIPS

CLEANDIRS= ${BINSRCDIRS} ${BINDIRS} ./sas ./dist ./cmds

SRCDIRS=  ./h 

CDSRCDIRS= ./conf ./fs ./net ./io ./sas ./sys ./vm ./data ./dist ./cmds ./consoles $(BINSRCDIRS)

ALLSRCDIRS= $(SRCDIRS) $(CDSRCDIRS)

ALLDIRS= $(ALLSRCDIRS) $(BINDIRS)

DESTROOT=/new.pool/Ultrix

DESTDIR=$(DESTROOT)/usr/sys


#
# Here's how and why this Makefile is written this way:
#
# Things that can be done on any machine do not
# contain the names of the machines (vax,mips) nor do they contain
# the tag $(MACHINE)
#
# Things that have to be done on the specific machine must contain
# the tag $(MACHINE) {or mips or vax if that is easier}
# note that using $(MACHINE) is prefered since that guarantees that
# the other architecture cannot execute those lines (unless you change MACHINE)
#

all: first.$(MACHINE) makes.$(MACHINE) sas-make dist-make

first.$(MACHINE): 
	cd machine/$(MACHINE);$(MAKE) first
	cd cmds; $(MAKE) first
	cd $(CAPMACHINE);$(MAKE) first

makes.$(MACHINE): 
	cd $(CAPMACHINE);$(MAKE) makes

sas-make:
	cd sas;$(MAKE) all

dist-make:
	cd dist;$(MAKE) all

tags$(MACHINE):
	cd $(CAPMACHINE);$(MAKE) tags

clean:
	-for i in $(CLEANDIRS) ;do \
		if [ -d $$i/. ]; then \
			$(ECHO) "$$i:";cd $$i;$(MAKE) clean;cd ..;\
		fi; \
	done

clobber:
	for i in $(CDSRCDIRS) $(MACHDIRS) ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) clobber;cd ..;done
	for i in $(ALLSRCDIRS) $(MACHDIRS); do \
		$(ECHO) "$$i:";cd $$i;sccs clean;cd ..;done

sccsinfo:
	-@echo -n .: ;sccs info
	-@for i in $(SRCDIRS); do \
		$(ECHO) "$$i:";cd $$i;sccs info;cd .. ; done
	-@for i in $(CDSRCDIRS) ; do \
		$(ECHO) "$$i:";cd $$i;sccs info;$(MAKE) sccsinfo;cd .. ; done

sccsget:
	@-for i in $(ALLSRCDIRS) $(MACHDIRS);do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;cd ..;done
	@-for i in $(CDSRCDIRS) $(MACHDIRS);do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) sccsget;cd ..;done

#took the all out of install...rr
insthdrs: instdirs inst-h inst-net inst-sys inst-fs inst-io \
	  inst-sas-hdrs inst-vm inst-$(MACHINE)-hdrs inst-common \
	  inst-net-hdrs 

install:  instdirs inst-conf inst-data  inst-h inst-io inst-net \
	  inst-sys inst-fs  inst-vm inst-cmds inst-consoles inst-sas \
	  inst-dist inst-$(MACHINE) inst-common

instdirs:
	-if [ -d ${DESTDIR} ] ;\
	then \
		rm -rf ${DESTDIR};\
	else true; \
	fi
	mkdir ${DESTDIR}
	/etc/chown root ${DESTDIR}
	chgrp system ${DESTDIR}
	chmod 0755 ${DESTDIR}
	rm -f $(DESTROOT)/sys
	cd $(DESTROOT); ln -s usr/sys $(DESTROOT)/sys
	-for i in $(ALLDIRS) ; do \
		if [ ! -d $(DESTROOT)/usr/sys/$$i ] ;\
		then \
			mkdir $(DESTROOT)/usr/sys/$$i; \
			/etc/chown root $(DESTROOT)/usr/sys/$$i; \
			chgrp system $(DESTROOT)/usr/sys/$$i; \
			chmod 0755 $(DESTROOT)/usr/sys/$$i; \
		else true; \
		fi; \
	done

inst-sas:
	echo Installing sas files
	cd sas;$(MAKE) DESTROOT=$(DESTROOT) install

inst-sas-hdrs:
	echo Installing sas headers
	cd sas;$(MAKE) DESTROOT=$(DESTROOT) insthdrs

inst-data:
	echo Installing data files
	cd data;$(MAKE) DESTROOT=$(DESTROOT) install

inst-vm:
	echo Installing vm files
	cd vm;$(MAKE) DESTROOT=$(DESTROOT) install

inst-dist:
	echo Installing dist files
	cd dist;$(MAKE) DESTROOT=$(DESTROOT) install

inst-h:
	echo Installing Header files
	cd h;chmod ugo+r *.h;tar cFf - *.h | ( cd $(DESTDIR)/h;tar xpf - )

inst-sys:
	echo Sys, this is easy!

inst-net:
	echo Installing net files
	cd net;$(MAKE) DESTROOT=$(DESTROOT) install

inst-net-hdrs:
	echo Installing net files
	cd net;$(MAKE) DESTROOT=$(DESTROOT) insthdrs

inst-fs:
	echo Installing fs files
	cd fs;$(MAKE) DESTROOT=$(DESTROOT) install

inst-io:
	echo Installing io files
	cd io;$(MAKE) DESTROOT=$(DESTROOT) install

inst-conf:
	echo Installing conf files
	cd conf; $(MAKE) DESTROOT=$(DESTROOT) install

inst-cmds:
	echo Installing kernel commands
	cd cmds; $(MAKE) DESTROOT=$(DESTROOT) install

inst-consoles:
	echo Installing console files
	cd consoles; $(MAKE) DESTROOT=$(DESTROOT) install

inst-$(MACHINE):
	echo Installing machine files
	cd machine/$(MACHINE);$(MAKE) DESTROOT=$(DESTROOT) install
	echo Installing CAPMACHINE files
	cd $(CAPMACHINE);$(MAKE) DESTROOT=$(DESTROOT) install

inst-$(MACHINE)-hdrs:
	cd machine/$(MACHINE);$(MAKE) DESTROOT=$(DESTROOT) insthdrs
	echo Installing CAPMACHINE files
	cd $(CAPMACHINE);$(MAKE) DESTROOT=$(DESTROOT) insthdrs

inst-common:
	echo Installing machine/common 
	cd machine/common;$(MAKE) DESTROOT=$(DESTROOT) MACHINE=$(MACHINE) install


#
#	@(#)Makefile	4.4 (ULTRIX) 3/6/91
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/machine/mips
#
#
# Files in the mips dir that go on the kit.
#
#
MIPS-FILES= genassym.c symbols.raw symbols.sort swap*.c vec_intr.c entry.s \
asm.h cachectl.h cca.h clock.h cons.h cons_sw.h cpu.h \
cpu_board.h debug.h dkio.h dvh.h entrypt.h fpu.h frame.h hwconf.h inst.h \
kn02ba.h kn210.h kn220.h kn5800.h mc146818clock.h mdcreg.h nexus.h param.h \
pcb.h ps_data.h psl.h pte.h r2350mem.h reg.h regdef.h s2681cons.h \
sableclock.h sablecons.h scb.h softfp.h ssc.h vmparam.h locore.s

all: first

clean:

clobber: clean rmlinks
	rm -f [abd-z]* [cim]*.*


sccsinfo:
	@sccs info

sccsget: 
	@-sccs get -s SCCS

insthdrs: instdirs inst-mips

install: instdirs inst-mips 

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
first:


inst-mips:
	@echo Installing mips files.
	tar cFf - ${MIPS-FILES} ${HEADER-FILES}|(cd ${DESTDIR};tar xpf -; chmod ugo+r * ; )

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

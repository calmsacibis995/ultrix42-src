# @(#)Makefile	4.1	(ULTRIX)	7/2/90
#
# Mid level generic makefile
#
all:
	-cd ${MACHINE};$(MAKE)

install:
	-cd ${MACHINE};$(MAKE) DESTROOT=${DESTROOT} install
	install -m 644 -c mop.h ${DESTROOT}/usr/sys/sas
	install -m 644 -c saio.h  ${DESTROOT}/usr/sys/sas

insthdrs:
	-cd ${MACHINE};$(MAKE) DESTROOT=${DESTROOT} insthdrs
	install -m 644 -c mop.h ${DESTROOT}/usr/sys/sas
	install -m 644 -c saio.h  ${DESTROOT}/usr/sys/sas

clean sccsinfo:
	-@echo -n " ${MACHINE}: "; cd ${MACHINE};$(MAKE) $@

sccsget:
	-@cd vax;${SCCS} get -s SCCS; $(MAKE) $@
	-@cd mips;${SCCS} get -s SCCS; $(MAKE) $@

clobber:
	-@cd vax;$(MAKE) $@; ${SCCS} clean
	-@cd mips;$(MAKE) $@; ${SCCS} clean
	-@${SCCS} clean


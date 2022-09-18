#	@(#)Makefile	4.2	(ULTRIX)	12/8/88
#
# Mid level generic makefile
#
all:
	-cd ${MACHINE};$(MAKE)

install: all
	-cd ${MACHINE};$(MAKE) DESTROOT=${DESTROOT} install

clean sccsinfo:
	-@echo -n " vax: "; cd vax;$(MAKE) $@
	-@echo -n " mips: "; cd mips;$(MAKE) $@

sccsget:
	-@cd vax;${SCCS} get -s SCCS; $(MAKE) $@
	-@cd mips;${SCCS} get -s SCCS; $(MAKE) $@

clobber:
	-@cd vax;$(MAKE) $@; ${SCCS} clean
	-@cd mips;$(MAKE) $@; ${SCCS} clean
	-@${SCCS} clean


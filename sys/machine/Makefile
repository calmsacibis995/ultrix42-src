#
#	@(#)Makefile	4.1 (ULTRIX)7/2/90
#
# Normal sequence of events is: clean clobber sccsget all install
#

DESTROOT=
DIRS=vax mips common

all:

clean: clean_$(MACHINE)

clean_$(MACHINE):
		if [ -d $(MACHINE) ]; then \
			$(ECHO) "$(MACHINE):";cd $(MACHINE);$(MAKE) clean;cd ..;\
		fi;


clobber: clean
	for i in $(DIRS) ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) clobber;cd ..;done

sccsinfo:
	-@for i in $(DIRS) ; do \
		$(ECHO) "$$i:";cd $$i;$(MAKE) sccsinfo;cd .. ; done

sccsget:
	@-for i in $(DIRS);do \
		$(ECHO) "$$i:";cd $$i;sccs get -s SCCS;$(MAKE) sccsget;cd ..;done


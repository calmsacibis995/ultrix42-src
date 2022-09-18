#  @(#)Makelocal.mk	2.2  ULTRIX  4/12/89

include $(GMAKEVARS)

DESTROOT=/new.pool/Ultrix
DESTDIR=$(DESTROOT)/usr/sys/cmds

OBJSMISC=main.o mkioconf.o mkmakefile.o mkubglue.o mkheaders.o mkswapconf.o \
	lconfig.o yconfig.o

CINCLUDES=-I. -I.. -I$(SRCROOT)/usr/include
LDLIBRARIES=-ll

all: rm_ytabh config
rm_ytabh:
	-if [ -f ../y.tab.h ] ;\
	then \
		$(RM) ../y.tab.h ; \
	else true; \
	fi

config: y.tab.h $(OBJSMISC)
	$(LDCMD) $(OBJSMISC) $(LDLIBRARIES)
main.o:		main.c
mkioconf.o:	mkioconf.c
mkmakefile.o:	mkmakefile.c
mkubglue.o:	mkubglue.c
mkheaders.o:	mkheaders.c
mkswapconf.o:	mkswapconf.c
lconfig.o:	lconfig.l
yconfig.o:	yconfig.y
y.tab.h: 
	yacc -d ../yconfig.y

install:
	-if [ ! -d ${DESTDIR}/config ] ;\
	then \
		mkdir ${DESTDIR}/config; \
		/etc/chown root ${DESTDIR}/config; \
		chgrp system ${DESTDIR}config; \
		chmod 0755 ${DESTDIR}/config; \
	else true; \
	fi
	-if [ ! -d ${DESTDIR}/config/_$(MACHINE).b ] ;\
	then \
		mkdir ${DESTDIR}/config/_$(MACHINE).b; \
		/etc/chown root ${DESTDIR}/config/_$(MACHINE).b; \
		chgrp system ${DESTDIR}/config/_$(MACHINE).b; \
		chmod 0755 ${DESTDIR}/config/_$(MACHINE).b; \
	else true; \
	fi
	$(INSTALL) -c -s config ${DESTDIR}/config/_$(MACHINE).b
	ln -s /sys/cmds/config/_$(MACHINE).b/config $(DESTROOT)/etc
	ln -s /sys/cmds/config/_$(MACHINE).b/config $(DESTROOT)/usr/etc

include $(GMAKERULES)

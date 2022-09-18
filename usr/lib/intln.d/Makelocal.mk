#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

LANGDBS= 646 8859 MCS
BASE= ${DESTROOT}/usr/lib/intln

646FILES=  Make646 ASCII.cod ASCII.in ENG.inc ENG_GB.646.in FRE.strtab \
	   FRE_FR.646.in GER.inc GER_DE.646.in ISO646.cod ISO646.inc \
	   ISO_646.in toupper.cnv tolower.cnv

8859FILES= Make8859 ENG_GB.8859.in FRE_FR.8859.in GER_DE.8859.in \
	   ISO8859.cod ascterm.cnv

MCSFILES=  MakeMCS ENG_GB.MCS.in FRE_FR.MCS.in GER_DE.MCS.in MCS.cod

SOURCES=   ${646FILES} ${8859FILES} ${MCSFILES}

ICONVDBS=  upper_lower

install :
	-mkdir ${BASE}
	chmod 755 ${BASE}
	-/etc/chown bin ${BASE}
	-chgrp system ${BASE}

	install -c -o bin -g system -m 444 ../help ${BASE}/help
	install -c -o bin -g system -m 444 ../patterns ${BASE}/patterns
	$(RM) ${DESTROOT}/usr/lib/intln/strextract
	ln -s ../../bin/strextract ${DESTROOT}/usr/lib/intln/strextract

	-(cd ${BASE};				\
		for i in ${LANGDBS}; do		\
			mkdir $$i;		\
			chmod 755 $$i;		\
			/etc/chown bin $$i;	\
			chgrp system $$i;	\
		done;				\
	);

	@for i in ${646FILES}; do		\
		$(ECHO) "$(INSTALL) -c -o bin -g system -m 444 ../$$i ${BASE}/646/$$i"; \
		$(INSTALL) -c -o bin -g system -m 444 ../$$i ${BASE}/646/$$i; \
	done;

	for i in ${8859FILES}; do		\
		install -c -o bin -g system -m 444 ../$$i ${BASE}/8859/$$i; \
	done;

	for i in ${MCSFILES}; do		\
		install -c -o bin -g system -m 444 ../$$i ${BASE}/MCS/$$i; \
	done;

	(cd ${BASE}/646; mv Make646 Makefile; make)
	(cd ${BASE}/8859; mv Make8859 Makefile; make)
	(cd ${BASE}/MCS; mv MakeMCS Makefile; make)

	-mkdir ${BASE}/conv
	chmod 755 ${BASE}/conv
	-/etc/chown bin ${BASE}/conv

	for i in ${ICONVDBS}; do		\
		$(INSTALL) -c -o bin -g system -m 444 ../$$i ${BASE}/conv/$$i; \
	done;

include $(GMAKERULES)

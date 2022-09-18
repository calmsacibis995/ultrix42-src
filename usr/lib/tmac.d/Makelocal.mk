# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
include $(GMAKEVARS)

FILES= tmac.a tmac.an tmac.an.new tmac.an.old tmac.an6n tmac.an6t tmac.bib  \
	tmac.cp tmac.e tmac.os tmac.r tmac.sdisp tmac.skeep tmac.srefs \
	tmac.vcat tmac.vgrind tmac.s tmac.an.nopage tmac.an.repro \
	tmac.an.4.3

all:

tools2 install: 
	-if [ ! -d ${DESTROOT}/usr/lib/tmac ] ; then \
		mkdir ${DESTROOT}/usr/lib/tmac; \
		chmod 755 ${DESTROOT}/usr/lib/tmac; \
		chown root ${DESTROOT}/usr/lib/tmac; \
		chgrp system ${DESTROOT}/usr/lib/tmac ;\
	else true; \
	fi
	for i in ${FILES}; do \
		echo copying $$i; \
		install -c -m 444 -g system ../$$i ${DESTROOT}/usr/lib/tmac/$$i ; \
	done

include $(GMAKERULES)

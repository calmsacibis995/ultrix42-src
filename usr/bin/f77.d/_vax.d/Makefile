# @(#)Makefile	4.1	ULTRIX	7/17/90
#

DESTROOT	      =

SUBDIR	      = src/f77 src/f77pass1 src/f1

all:		${SUBDIR}

${SUBDIR}:	/tmp
		cd $@; make ${MFLAGS}

install:
		-if [ ! -d ${DESTROOT}/usr/ucb ] ;\
		then \
			mkdir ${DESTROOT}/usr/ucb; \
			/etc/chown root ${DESTROOT}/usr/ucb; \
			chgrp system ${DESTROOT}/usr/ucb; \
			chmod 0755 ${DESTROOT}/usr/ucb ; \
		else true ; \
		fi
		for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTROOT=${DESTROOT} install); done

tools2:
		-if [ ! -d ${DESTROOT}/usr/ucb ] ;\
		then \
			mkdir ${DESTROOT}/usr/ucb; \
			/etc/chown root ${DESTROOT}/usr/ucb; \
			chgrp system ${DESTROOT}/usr/ucb; \
			chmod 0755 ${DESTROOT}/usr/ucb ; \
		else true ; \
		fi
		for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTROOT=${DESTROOT} install); done

cleanvax clean:
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

clobber: 
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clobber); done
	(cd include; rm -f *.h); 
	rm -f Makefile

sccsinfo:
	sccs info
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} sccsinfo); done
	(cd include; sccs info); 

sccsget:
	sccs get Makefile
	for i in ${SUBDIR}; do (cd $$i;sccs get Makefile; make ${MFLAGS} sccsget); done
	(cd include; sccs get SCCS); 

depend:
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} depend); done

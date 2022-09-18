#  @(#)Makefile.mk	4.1  ULTRIX  7/2/90

DESTROOT=
LESSONS=Init L0 L1.1a L1.1b L1.2 L2.1 L2.2 L2.3 L2.4 L3.1 \
	L3.2 L3.3 L3.4 L4.0 L4.1 L4.2 L4.3 L5.1 L5.2 L5.3 L5.4 \
	L6.1 L6.2 L6.3 L6.4 longtext Makefile README

all:

install: ${LESSONS}
	@echo Install ../new/learn
	@-if [ ! -d ${DESTROOT}/usr/lib/learn/vi ] ; then \
		mkdir ${DESTROOT}/usr/lib/learn/vi ; \
		chmod 755 ${DESTROOT}/usr/lib/learn/vi; \
		/etc/chown root ${DESTROOT}/usr/lib/learn/vi; \
		chgrp staff ${DESTROOT}/usr/lib/learn/vi; \
	else true; \
	fi
	chmod 555 Init
	@tar cf - ${LESSONS} | (cd ${DESTROOT}/usr/lib/learn/vi; tar xpf -)

clean:
clobber: clean
	@rm -f L* L*.* README l* Makefile Init
sccsget:
		@sccs get -s SCCS

sccsinfo:
		@echo -n ../new/learn:
		@sccs info

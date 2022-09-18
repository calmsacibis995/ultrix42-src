#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

#
# /usr/examples/snmp/snmpext Makefile
#

DESTLIST= ${DESTROOT}/usr/examples/snmp/snmpext

STD=	defs.h diskdef.h vmdef.h main.c disk_grp.c snmp.c vm_grp.c

install:
	for i in ${STD}; do \
	${INSTALL} -c -m 444 ../$$i ${DESTROOT}/usr/examples/snmp/snmpext/$$i; \
	done; \
	${INSTALL} -c -m 444 ../makefile.ex ${DESTROOT}/usr/examples/snmp/snmpext/Makefile

include $(GMAKERULES)

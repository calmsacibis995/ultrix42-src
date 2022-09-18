#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST= ${DESTROOT}/usr/examples/snmp

SUBDIRS=snmpext

include $(GMAKERULES)

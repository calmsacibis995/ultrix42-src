#  @(#)Makelocal.mk	4.4  ULTRIX  2/21/91

include $(GMAKEVARS)

DESTLIST= ${DESTROOT}/usr/examples

SUBDIRS=lat itc snmp dli xti dbx devdrivers

include $(GMAKERULES)

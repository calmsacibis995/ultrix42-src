# @(#)Makelocal.mk	4.1	(ULTRIX)	3/6/91

include $(GMAKEVARS)

#
# /var/dss/kerberos/dbase/krb_push script
#

DESTLIST= ${DESTROOT}/var/dss/kerberos/dbase

install:
	${INSTALL} -c -m 755 ../krb_push ${DESTROOT}/var/dss/kerberos/dbase/krb_push

include $(GMAKERULES)

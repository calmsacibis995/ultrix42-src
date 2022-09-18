# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
#
# Modification History:
#
# 16-Oct-89	Williams
#	Removed usage of /etc/named
#
# 01-Aug-89	logcher
#	Removed named.hosts and named.rev and FILES as well since 
#	these are now created in bindsetup.
#
# 12-Jun-89	logcher
#	Removed named.ca and named.local from FILES now that 
#	bindsetup makes them.  Added DESTDIR and SCRIPT.
#

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/var/dss/namedb/bin \
	$(DESTROOT)/usr/var/dss/namedb/src

DESTDIR=$(DESTROOT)/usr/var/dss/namedb

SCRIPT=make_aliases make_auth make_group make_hosts make_networks \
	make_passwd make_protocols make_rpc make_services restart_named

install:
	-for i in $(SCRIPT); do \
		$(INSTALL) -c -m 700 ../$$i.sh ${DESTDIR}/bin/$$i; \
	done
	$(INSTALL) -c -m 700 ../make.script ${DESTDIR}/Makefile

include $(GMAKERULES)

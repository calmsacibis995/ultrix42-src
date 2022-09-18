#  @(#)Makelocal.mk	4.2  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc $(DESTROOT)/usr $(DESTROOT)/usr/etc

SUBDIRS= disktab.d fstab.d group.d license.d passwd.d rc.d

DATA_644= elcsd.conf ftpusers gettytab hosts hosts.equiv inetd.conf \
	motd networks ntp.conf printcap protocols remote rpc services shells \
	sliphosts snmpd.conf svc.conf ttys

DATA_600= snmpd.conf

install:
	@for i in $(DATA_644); \
	do \
		echo "$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/etc/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(DESTROOT)/etc/$$i; \
	done; \
	for i in $(DATA_600); \
	do \
		echo "$(INSTALL) -c -m 600 ../$$i $(DESTROOT)/etc/$$i"; \
		$(INSTALL) -c -m 600 ../$$i $(DESTROOT)/etc/$$i; \
	done

include $(GMAKERULES)

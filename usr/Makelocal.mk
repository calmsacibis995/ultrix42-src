#  @(#)Makelocal.mk	4.4	ULTRIX  2/28/91

include $(GMAKEVARS)

SUBDIRS = bin etc examples lib ucb games new old mdec pub spool skel doc man

#Install is essentially handwired right now.  
#It does lots of misc. things while the real work is in
#usr/lib & usr/bin's Makefiles

install: install2 $(SUBDIRS) install_kits

install2:
	-if [ ! -d ${DESTROOT}/usr/tmp ]; \
	then \
		mkdir -p ${DESTROOT}/usr/tmp; \
		chmod 777 ${DESTROOT}/usr/tmp; \
	else \
		true; \
	fi

	-if [ ! -d ${DESTROOT}/usr/mdec ]; \
	then \
		mkdir -p ${DESTROOT}/usr/mdec; \
		chmod 755 ${DESTROOT}/usr/mdec; \
	else \
		true; \
	fi

	-if [ ! -d ${DESTROOT}/usr/ucb ]; \
	then \
		mkdir -p ${DESTROOT}/usr/ucb; \
		chmod 755 ${DESTROOT}/usr/ucb; \
	else \
		true; \
	fi

	-if [ ! -d ${DESTROOT}/usr/var/adm ]; \
	then \
		mkdir -p ${DESTROOT}/usr/var/adm; \
		chmod 755 ${DESTROOT}/usr/var/adm; \
		/etc/chown root ${DESTROOT}/usr/var/adm; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/adm/crash ]; \
	then \
		mkdir ${DESTROOT}/usr/var/adm/crash; \
		chmod 755 ${DESTROOT}/usr/var/adm/crash; \
		/etc/chown root ${DESTROOT}/usr/var/adm/crash; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/adm/syserr ]; \
	then \
		mkdir ${DESTROOT}/usr/var/adm/syserr; \
		chmod 755 ${DESTROOT}/usr/var/adm/syserr; \
		/etc/chown root ${DESTROOT}/usr/var/adm/syserr; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/adm/snap ]; \
	then \
		mkdir ${DESTROOT}/usr/var/adm/snap; \
		chmod 755 ${DESTROOT}/usr/var/adm/snap; \
		/etc/chown root ${DESTROOT}/usr/var/adm/snap; \
	else \
		true; \
	fi
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/acct
	install -c -m 600 -o uucp /dev/null ${DESTROOT}/usr/var/adm/aculog
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/lastlog
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/lpd-errs
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/messages
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/msgbuf
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/shutdownlog
	install -c -m 644 /dev/null ${DESTROOT}/usr/var/adm/wtmp
	-if [ ! -d ${DESTROOT}/tmp ]; \
	then \
		mkdir ${DESTROOT}/tmp; \
		chmod 777 ${DESTROOT}/tmp; \
		/etc/chown root ${DESTROOT}/tmp; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/mnt ]; \
	then \
		mkdir ${DESTROOT}/mnt; \
		chmod 755 ${DESTROOT}/mnt; \
		/etc/chown root ${DESTROOT}/mnt; \
	else \
		true; \
	fi
	install -c -o root /dev/null ${DESTROOT}/usr/restoresymtable
	-if [ ! -d ${DESTROOT}/usr/users ]; \
	then \
		mkdir ${DESTROOT}/usr/users; \
	else \
		true; \
	fi

	-if [ ! -d ${DESTROOT}/library ]; \
	then \
		mkdir ${DESTROOT}/library; \
	else \
		true; \
	fi
	-if [ ! -d $(DESTROOT)/lib ]; then \
		ln -s usr/lib $(DESTROOT)/lib; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/sys ]; then \
		ln -s usr/sys $(DESTROOT)/sys; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/var ]; then \
		ln -s usr/var $(DESTROOT)/var; \
	else true; \
	fi

install_kits:
	@echo 'install kits'
	-if [ ! -d ${DESTROOT}/usr/kits ]; \
	then \
		mkdir ${DESTROOT}/usr/kits; \
	else \
		true; \
	fi
	(cd ../kits; $(MAKE) DESTROOT=${DESTROOT} install)


include $(GMAKERULES)

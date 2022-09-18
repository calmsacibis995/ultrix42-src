#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

SUBDIRS=include adm lib src aux md cf doc adb test

DIRS=	$(DESTROOT)/usr/var/spool/mqueue
SRCDIR=	$(DESTROOT)/usr/src/usr.lib/sendmail

install:
	@-if [ ! -d $(DESTROOT)/usr/var/adm ]; then \
		mkdir $(DESTROOT)/usr/var/adm; \
		chmod 755 $(DESTROOT)/usr/var/adm; \
		/etc/chown root $(DESTROOT)/usr/var/adm; \
		chgrp system $(DESTROOT)/usr/var/adm; \
	else true; \
	fi
	@-if [ ! -d $(DESTROOT)/usr/var/spool/mqueue ]; then \
		mkdir $(DESTROOT)/usr/var/spool/mqueue; \
		chmod 755 $(DESTROOT)/usr/var/spool/mqueue; \
		/etc/chown root $(DESTROOT)/usr/var/spool/mqueue; \
		chgrp system $(DESTROOT)/usr/var/spool/mqueue; \
	else true; \
	fi
	@-if [ ! -d $(DESTROOT)/usr/ucb ]; then \
		mkdir $(DESTROOT)/usr/ucb; \
		chmod 755 $(DESTROOT)/usr/ucb; \
		/etc/chown root $(DESTROOT)/usr/ucb; \
		chgrp system $(DESTROOT)/usr/ucb; \
	else true; \
	fi
	install -c -s -o daemon ../aux/_$(MACHINE).b/syslog $(DESTROOT)/usr/etc/syslog
	$(RM) $(DESTROOT)/etc/syslog
	ln -s ../usr/etc/syslog	$(DESTROOT)/etc/syslog
	install -c -o daemon -m 644 ../aux/syslog.conf $(DESTROOT)/etc/syslog.conf
	$(RM) $(DESTROOT)/usr/lib/syslog.conf
	ln -s ../../etc/syslog.conf $(DESTROOT)/usr/lib/syslog.conf
	install -c -o daemon -m 644 /dev/null $(DESTROOT)/etc/syslog.pid
	$(RM) $(DESTROOT)/usr/lib/syslog.pid
	ln -s ../../etc/syslog.pid $(DESTROOT)/usr/lib/syslog.pid
	install -c -o daemon -m 644 /dev/null $(DESTROOT)/usr/var/spool/mqueue/syslog
	for i in 0 1 2 3 4 5 6 7; do \
		install -c -o daemon -m 644 /dev/null $(DESTROOT)/usr/var/spool/mqueue/syslog.$$i; \
	done
	install -c -o daemon ../lib/newsyslog.sh $(DESTROOT)/usr/var/adm/newsyslog
#	echo	'5 4 * * * sh /usr/var/adm/newsyslog' >> $(DESTROOT)/usr/lib/crontab
	install -c -m 644 ../lib/aliases $(DESTROOT)/etc/aliases 
	$(RM) $(DESTROOT)/usr/lib/aliases
	ln -s ../../etc/aliases	$(DESTROOT)/usr/lib/aliases
	install -c -m 644 ../lib/aliases.dir $(DESTROOT)/etc/aliases.dir
	$(RM) $(DESTROOT)/usr/lib/aliases.dir
	ln -s ../../etc/aliases.dir	$(DESTROOT)/usr/lib/aliases.dir
	install -c -m 644 ../lib/aliases.pag $(DESTROOT)/etc/aliases.pag	
	$(RM) $(DESTROOT)/usr/lib/aliases.pag
	ln -s ../../etc/aliases.pag	$(DESTROOT)/usr/lib/aliases.pag
	install -c ../lib/mail.aliases $(DESTROOT)/etc/mail.aliases
	$(RM) $(DESTROOT)/usr/lib/mail.aliases
	ln -s ../../etc/mail.aliases	$(DESTROOT)/usr/lib/mail.aliases
	install -c -s -m 4755 ../src/_$(MACHINE).b/sendmail	$(DESTROOT)/usr/lib/sendmail
	install -c -m 644 ../lib/sendmail.hf	$(DESTROOT)/etc/sendmail.hf
	$(RM) $(DESTROOT)/usr/lib/sendmail.hf
	ln -s ../../etc/sendmail.hf	$(DESTROOT)/usr/lib/sendmail.hf
	install -c -m 644	/dev/null	$(DESTROOT)/etc/sendmail.st
	$(RM) $(DESTROOT)/usr/lib/sendmail.st
	ln -s ../../etc/sendmail.st	$(DESTROOT)/usr/lib/sendmail.st
	install -c -m 600	/dev/null	$(DESTROOT)/etc/sendmail.fc
	$(RM) $(DESTROOT)/usr/lib/sendmail.fc
	ln -s ../../etc/sendmail.fc	$(DESTROOT)/usr/lib/sendmail.fc
	install -c -m 644 ../cf/_$(MACHINE).b/exampleether.cf	$(DESTROOT)/etc/sendmail.cf
	$(RM) $(DESTROOT)/usr/lib/sendmail.cf
	ln -s ../../etc/sendmail.cf	$(DESTROOT)/usr/lib/sendmail.cf
	rm -f $(DESTROOT)/usr/ucb/newaliases
	$(LN) -s ../../usr/lib/sendmail $(DESTROOT)/usr/ucb/newaliases
	rm -f $(DESTROOT)/usr/ucb/mailq
	$(LN) -s ../../usr/lib/sendmail $(DESTROOT)/usr/ucb/mailq

include $(GMAKERULES)

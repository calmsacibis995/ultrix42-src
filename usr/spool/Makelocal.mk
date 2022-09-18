#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
# Makefile for the spool directory
# 27-Feb-87  Teoman Topcubasi, added the installation of /usr/spool/secretmail
#		and the file /usr/spool/secretmail/notice
#
include $(GMAKEVARS)

all:

install:
	-if [ ! -d ${DESTROOT}/usr/spool ]; \
	then \
		mkdir ${DESTROOT}/usr/spool; \
		chmod 755 ${DESTROOT}/usr/spool; \
		/etc/chown root ${DESTROOT}/usr/spool; \
		chgrp system ${DESTROOT}/usr/spool; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/secretmail ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/secretmail; \
		chmod 777 ${DESTROOT}/usr/spool/secretmail; \
		/etc/chown root ${DESTROOT}/usr/spool/secretmail; \
		chgrp system ${DESTROOT}/usr/spool/secretmail; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/locks ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/locks; \
		chmod 755 ${DESTROOT}/usr/spool/locks; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/mail ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/mail; \
		chmod 777 ${DESTROOT}/usr/spool/mail; \
		chmod +t ${DESTROOT}/usr/spool/mail; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/rje ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/rje; \
		chmod 755 ${DESTROOT}/usr/spool/rje; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/lpd ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/lpd; \
		chmod 755 ${DESTROOT}/usr/spool/lpd; \
		/etc/chown daemon ${DESTROOT}/usr/spool/lpd; \
		chgrp system ${DESTROOT}/usr/spool/lpd; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/spool/rwho ]; \
	then \
		mkdir ${DESTROOT}/usr/spool/rwho; \
		chmod 755 ${DESTROOT}/usr/spool; \
		/etc/chown root ${DESTROOT}/usr/spool/rwho; \
		chgrp system ${DESTROOT}/usr/spool/rwho; \
	else \
		true; \
	fi
	install -c -m 644 ../lock ${DESTROOT}/usr/spool/lpd/lock
	install -c -m 644 ../notice ${DESTROOT}/usr/spool/secretmail/notice

include $(GMAKERULES)

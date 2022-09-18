# @(#)Makelocal.mk	4.2   (ULTRIX)        9/7/90
# var Makefile  -- jhw
#
# 001 - John Williams 17-mar-87
#	install changes
#
# 002 - Mark Parenti 28-Feb-89
#	Update for new generic makefiles.
#
# 003 - Suzanne Logcher 08-Jun-89
#	Added /usr/var/dss, /usr/var/dss/namedb, /usr/var/dss/kerberos,
#	/usr/var/dss/ncs.
#
# 004 - Terry Linsey 06-Sep-90
#       Added /var/yp/src.
#
include $(GMAKEVARS)
#DESTROOT= 


install:  
	-if [ ! -d ${DESTROOT}/bin ] ; then \
		mkdir ${DESTROOT}/bin; \
		chmod 755 ${DESTROOT}/bin; \
		/etc/chown root ${DESTROOT}/bin; \
		chgrp system ${DESTROOT}/bin; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/tmp ] ; then \
		mkdir ${DESTROOT}/tmp; \
		chmod 1777 ${DESTROOT}/tmp; \
		/etc/chown root ${DESTROOT}/tmp; \
		chgrp system ${DESTROOT}/tmp; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr ] ; then \
		mkdir ${DESTROOT}/usr; \
		chmod 755 ${DESTROOT}/usr; \
		/etc/chown root ${DESTROOT}/usr; \
		chgrp system ${DESTROOT}/usr; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var ] ; then \
		mkdir ${DESTROOT}/usr/var; \
		chmod 755 ${DESTROOT}/usr/var; \
		/etc/chown root ${DESTROOT}/usr/var; \
		chgrp system ${DESTROOT}/usr/var; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/tmp ] ; then \
		mkdir ${DESTROOT}/usr/var/tmp; \
		chmod 777 ${DESTROOT}/usr/var/tmp; \
		/etc/chown root ${DESTROOT}/usr/var/tmp; \
		chgrp system ${DESTROOT}/usr/var/tmp; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/adm ] ; then \
		mkdir ${DESTROOT}/usr/var/adm; \
		chmod 755 ${DESTROOT}/usr/var/adm; \
		/etc/chown root ${DESTROOT}/usr/var/adm; \
		chgrp system ${DESTROOT}/usr/var/adm; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/spool ] ; then \
		mkdir ${DESTROOT}/usr/var/spool; \
		chmod 755 ${DESTROOT}/usr/var/spool; \
		/etc/chown root ${DESTROOT}/usr/var/spool; \
		chgrp system ${DESTROOT}/usr/var/spool; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/dss ] ; then \
		mkdir ${DESTROOT}/usr/var/dss; \
		chmod 755 ${DESTROOT}/usr/var/dss; \
		/etc/chown root ${DESTROOT}/usr/var/dss; \
		chgrp system ${DESTROOT}/usr/var/dss; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/dss/namedb ] ; then \
		mkdir ${DESTROOT}/usr/var/dss/namedb; \
		chmod 755 ${DESTROOT}/usr/var/dss/namedb; \
		/etc/chown root ${DESTROOT}/usr/var/dss/namedb; \
		chgrp system ${DESTROOT}/usr/var/dss/namedb; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/dss/kerberos ] ; then \
		mkdir ${DESTROOT}/usr/var/dss/kerberos; \
		chmod 755 ${DESTROOT}/usr/var/dss/kerberos; \
		/etc/chown root ${DESTROOT}/usr/var/dss/kerberos; \
		chgrp system ${DESTROOT}/usr/var/dss/kerberos; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/dss/ncs ] ; then \
		mkdir ${DESTROOT}/usr/var/dss/ncs; \
		chmod 755 ${DESTROOT}/usr/var/dss/ncs; \
		/etc/chown root ${DESTROOT}/usr/var/dss/ncs; \
		chgrp system ${DESTROOT}/usr/var/dss/ncs; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/yp ] ; then \
		mkdir ${DESTROOT}/usr/var/yp; \
		chmod 755 ${DESTROOT}/usr/var/yp; \
		/etc/chown root ${DESTROOT}/usr/var/yp; \
		chgrp system ${DESTROOT}/usr/var/yp; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/var/yp/src ] ; then \
		mkdir ${DESTROOT}/usr/var/yp/src; \
		chmod 755 ${DESTROOT}/usr/var/yp/src; \
                /etc/chown root ${DESTROOT}/usr/var/yp/src; \
                chgrp system ${DESTROOT}/usr/var/yp/src; \
                else true; \
        fi
	-if [ ! -d ${DESTROOT}/usr ] ; then \
		mkdir ${DESTROOT}/usr; \
		chmod 755 ${DESTROOT}/usr; \
		/etc/chown root ${DESTROOT}/usr; \
		chgrp system ${DESTROOT}/usr; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/etc ] ; then \
		mkdir ${DESTROOT}/usr/etc; \
		chmod 755 ${DESTROOT}/usr/etc; \
		/etc/chown root ${DESTROOT}/usr/etc; \
		chgrp system ${DESTROOT}/usr/etc; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/bin ] ; then \
		mkdir ${DESTROOT}/usr/bin; \
		chmod 755 ${DESTROOT}/usr/bin; \
		/etc/chown root ${DESTROOT}/usr/bin; \
		chgrp system ${DESTROOT}/usr/bin; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/lib ] ; then \
		mkdir ${DESTROOT}/usr/lib; \
		chmod 755 ${DESTROOT}/usr/lib; \
		/etc/chown root ${DESTROOT}/usr/lib; \
		chgrp system ${DESTROOT}/usr/lib; \
		else true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/sys ] ; then \
		mkdir ${DESTROOT}/usr/sys; \
		chmod 755 ${DESTROOT}/usr/sys; \
		/etc/chown root ${DESTROOT}/usr/sys; \
		chgrp system ${DESTROOT}/usr/sys; \
		else true; \
	fi
	-if [  -d ${DESTROOT}/usr/spool ] ; then \
		rm -rf ${DESTROOT}/usr/spool; \
		else true; \
	fi
	ln -s var/spool ${DESTROOT}/usr/spool
	-if [  -d ${DESTROOT}/usr/adm ] ; then \
		rm -rf ${DESTROOT}/usr/adm; \
		else true; \
	fi
	ln -s var/adm ${DESTROOT}/usr/adm
	-if [  -d ${DESTROOT}/usr/tmp ] ; then \
		rm -rf ${DESTROOT}/usr/tmp; \
		else true; \
	fi
	ln -s var/tmp ${DESTROOT}/usr/tmp

include $(GMAKERULES)

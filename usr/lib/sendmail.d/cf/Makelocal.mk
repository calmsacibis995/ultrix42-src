#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

######################################################################
#
#	Makefile for system generation handling of
#	sendmail configuration files.
#
#		@(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
#
######################################################################

MAILERS= localm.m4 uucpm.m4 tcpm.m4 umc.m4 xm.m4
BASES= version.m4 base.m4 localbase.m4 zerobase.m4

EXFILES= generic.mc

EXMAKE= Makefile.dist

GENSRCS= base.m4 generic.mc localbase.m4 localm.m4 tcpm.m4 uucpm.m4 \
	version.m4 umc.m4 xm.m4 zerobase.m4

M4SRCS= base.tag localbase.tag localm.tag tcpm.tag uucpm.tag \
	version.tag umc.tag xm.tag zerobase.tag

SRCS= $(EXFILES) $(EXMAKE) $(GENSRCS)

EXAMPLE= generic.cf exampleuucp.cf exampleether.cf

COPY= $(EXFILES) $(GENSRCS)

GET=	sccs get

.SUFFIXES: .mc .cf

.mc.cf:
	rm -f $*.cf
	m4 ../$*.mc > $*.cf
	chmod 755 $*.cf
	cp $*.cf exampleether.cf
	cp $*.cf exampleuucp.cf

all: $(EXAMPLE)

exampleuucp.cf exampleether.cf: generic.cf

generic.cf:	generic.mc $(BASES) $(MAILERS)

generic.mc: $(M4SRCS)

$(M4SRCS):
	$(RM) `basename $@ .tag`.m4
	$(LN) ../`basename $@ .tag`.m4 .

#####################
#   administrivia   #
#####################

install:
	-if [ ! -d $(DESTROOT)/usr/src ]; then\
		mkdir $(DESTROOT)/usr/src; \
		/etc/chown root $(DESTROOT)/usr/src; \
		chgrp system $(DESTROOT)/usr/src; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/usr/src/usr.lib ]; then\
		mkdir $(DESTROOT)/usr/src/usr.lib ; \
		/etc/chown root $(DESTROOT)/usr/src/usr.lib ; \
		chgrp system $(DESTROOT)/usr/src/usr.lib ; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/usr/src/usr.lib/sendmail ]; then\
		mkdir $(DESTROOT)/usr/src/usr.lib/sendmail ; \
		/etc/chown root $(DESTROOT)/usr/src/usr.lib/sendmail ; \
		chgrp system $(DESTROOT)/usr/src/usr.lib/sendmail ; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/usr/src/usr.lib/sendmail/cf ]; then\
		mkdir $(DESTROOT)/usr/src/usr.lib/sendmail/cf ; \
		/etc/chown root $(DESTROOT)/usr/src/usr.lib/sendmail/cf ; \
		chgrp system $(DESTROOT)/usr/src/usr.lib/sendmail/cf ; \
	else true; \
	fi
	-if [ ! -d $(DESTROOT)/usr/spool/mqueue ]; then\
		mkdir $(DESTROOT)/usr/spool/mqueue ; \
	else true; \
	fi
	-/etc/chown daemon $(DESTROOT)/usr/spool/mqueue
	-chgrp system $(DESTROOT)/usr/spool/mqueue
	-chmod 755 $(DESTROOT)/usr/spool/mqueue 
	-if [ ! -d $(DESTROOT)/usr/spool/mail ]; then\
		mkdir $(DESTROOT)/usr/spool/mail ; \
	else true; \
	fi
	-/etc/chown root $(DESTROOT)/usr/spool/mail
	-chgrp system $(DESTROOT)/usr/spool/mail
	-chmod 777 $(DESTROOT)/usr/spool/mail 
	-chmod +t $(DESTROOT)/usr/spool/mail 
	-for i in $(EXAMPLE) ; do \
		install -c $$i $(DESTROOT)/usr/src/usr.lib/sendmail/cf/$$i; \
	done
	-for i in $(COPY) ; do \
		install -c ../$$i $(DESTROOT)/usr/src/usr.lib/sendmail/cf/$$i; \
	done
	-install -c ../Makefile.dist $(DESTROOT)/usr/src/usr.lib/sendmail/cf/Makefile 
	cp generic.cf $(DESTROOT)/usr/lib/sendmail.cf

include $(GMAKERULES)

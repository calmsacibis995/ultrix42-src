#	Makelocal.mk
#
#	@(#)Makelocal.mk	4.2	(ULTRIX)	3/5/91
#
#	000	20-feb-1991	overman
#	New.
#
include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc/fis
SUBDIRS= kernel.d

ETCFILES= DEP_ORD fis_.login fis_default.DECterm fis_rc.local fisinit \
	  fisstart fisprep fisld tifis

install:	$(ETCFILES)

	-if [ ! -d ${DESTLIST} ]; \
	then \
		mkdir -p ${DESTLIST}; \
		chmod 755 ${DESTLIST}; \
	else \
		true; \
	fi

	@for i in $(ETCFILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 755 ../$$i $(DESTLIST)/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(DESTLIST)/$$i; \
	done

lint:

include $(GMAKERULES)

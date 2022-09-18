#	Makelocal.mk
#
#	@(#)Makelocal.mk	4.1	(ULTRIX)	2/28/91
#
#	000	20-feb-1991	Wallace
#	New.
#
include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc/fis

ETCFILES= options.mips pseudo.mips

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

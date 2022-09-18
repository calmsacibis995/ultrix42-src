#	Makelocal.mk
#
#	@(#)Makelocal.mk	4.1	(ULTRIX)	2/28/91
#
#	000	20-feb-1991	overman
#	New.
#
include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/etc/subsets

FILES= COMU4BACKEND205.ctrl COMU4BACKEND205.inv  COMU4BACKEND205.scp \
       COMU4BACKEND205.lk

install:	$(FILES)

	-if [ ! -d ${DESTLIST} ]; \
	then \
		mkdir -p ${DESTLIST}; \
		chmod 755 ${DESTLIST}; \
	else \
		true; \
	fi

	@for i in $(FILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 644 ../$$i $(DESTLIST)/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(DESTLIST)/$$i; \
	done

lint:

include $(GMAKERULES)

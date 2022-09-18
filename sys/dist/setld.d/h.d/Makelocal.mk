#	Makelocal.mk
#		sys/dist/setld.d/h.d Makefile
#
#  "@(#)Makelocal.mk	4.1 (ULTRIX) 7/2/90"
#
#	000	03-mar-1989	ccb
#	New.
#
#	001	19-jun-1989	ccb
#		add dummy lint rule

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/include/setld
HFILES= list.h setld.h

all:		$(HFILES)

lint:

install:	$(HFILES)
	@for i in $(HFILES); \
	do \
		$(ECHO) "$(INSTALL) -c -m 644 ../$$i $(DESTLIST)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(DESTLIST)/$$i; \
	done

include $(GMAKERULES)


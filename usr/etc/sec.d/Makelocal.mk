# @(#)Makelocal.mk	2.3	(ULTRIX)	6/8/89
#
include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/usr/etc/sec
DESTLIST=$(DESTDIR)
SYDIR=../../../../sys/sys
SUBDIRS=getauth.d edauth.d

AOUTS=audgen
OBJS=audit_tool
audgen:		audgen.o
audgen.o:	audgen.c

audit_tool:	audit_tool.o
		$(LDCMD) audit_tool.o syscalls.o -laud
audit_tool.o:	audit_tool.c
		$(CCCMD) ../audit_tool.c $(SYDIR)/syscalls.c

syscalls.o:	$(SYDIR)/syscalls.c 
	$(CCCMD) $(SYDIR)/syscalls.c

install:
	-$(INSTALL) -c -s -m 500 -o root audgen $(DESTLIST)/audgen
	-$(INSTALL) -c -s -m 500 -o root audit_tool $(DESTLIST)/audit_tool
	-$(INSTALL) -c -m 500 -o root ../secsetup $(DESTLIST)/secsetup


include $(GMAKERULES)


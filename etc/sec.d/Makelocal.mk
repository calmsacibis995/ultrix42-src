# @(#)Makelocal.mk	2.3	(ULTRIX)	6/8/89
#
include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/etc/sec
DESTLIST=$(DESTDIR)
SYDIR=../../../sys/sys

OBJS= auditd auditmask

auditd:		auditd.o
		$(LDCMD) auditd.o -lkrb -ldes -lknet
auditd.o:       auditd.c
		$(CCCMD) -DKERBEROS ../auditd.c

auditmask:	auditmask.o
		$(LDCMD) auditmask.o syscalls.o -laud
auditmask.o:	auditmask.c
		$(CCCMD) ../auditmask.c $(SYDIR)/syscalls.c

syscalls.o: $(SYDIR)/syscalls.c 
	$(CCCMD) $(SYDIR)/syscalls.c

install:
	-$(INSTALL) -c -s -m 500 -o root auditd $(DESTLIST)/auditd
	-$(INSTALL) -c -s -m 500 -o root auditmask $(DESTLIST)/auditmask
	cp ../audit_events .
	-$(INSTALL) -c -m 400 -o root audit_events $(DESTLIST)/audit_events

include $(GMAKERULES)

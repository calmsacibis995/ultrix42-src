# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

LOADLIBES = -lc

AOUT=lockd

OBJS=	prot_main.o prot_lock.o prot_pklm.o prot_pnlm.o prot_msg.o\
	prot_proc.o prot_libr.o prot_alloc.o prot_priv.o\
	tcp.o udp.o xdr_nlm.o xdr_klm.o xdr_sm.o sm_monitor.o \
	svc_udp.o hash.o prot_freeall.o prot_share.o

prot_main.o:	prot_main.c
prot_lock.o:	prot_lock.c
prot_pklm.o:	prot_pklm.c
prot_pnlm.o:	prot_pnlm.c
prot_msg.o:	prot_msg.c
prot_proc.o:	prot_proc.c
prot_libr.o:	prot_libr.c
prot_alloc.o:	prot_alloc.c
prot_priv.o:	prot_priv.c
tcp.o:	tcp.c
udp.o:	udp.c
xdr_nlm.o:	xdr_nlm.c
xdr_klm.o:	xdr_klm.c
xdr_sm.o:	xdr_sm.c
sm_monitor.o:	sm_monitor.c
svc_udp.o:	svc_udp.c
hash.o:	hash.c
prot_freeall.o:	prot_freeall.c
prot_share.o:	prot_share.c

AOUTS=	nfssetlock

nfssetlock:	nfssetlock.o
nfssetlock.o:	nfssetlock.c

install:
	$(INSTALL) -c -s lockd $(DESTROOT)/usr/etc/lockd
	$(INSTALL) -c -s nfssetlock $(DESTROOT)/usr/etc/nfssetlock


include $(GMAKERULES)

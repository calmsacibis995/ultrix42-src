# @(#)Makelocal.mk	4.1 	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	rpc.statd

OBJS=	sm_svc.o sm_proc.o sm_statd.o tcp.o

tcp.o: ../../lockd.d/tcp.c
	$(CCCMD) ../../lockd.d/tcp.c

sm_svc.o:	sm_svc.c
sm_proc.o:	sm_proc.c
sm_statd.o:	sm_statd.c

install: 
	$(INSTALL) -c -s rpc.statd ${DESTROOT}/usr/etc/statd

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.3  ULTRIX  2/28/91

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUTS=	presto prestoctl_svc

presto:		presto.o prestoctl_clnt.o prestoctl_xdr.o
prestoctl_svc:	prestoctl_proc.o prestoctl_svc.o prestoctl_xdr.o

presto.o:		presto.c 
prestoctl_clnt.o:	prestoctl_clnt.c
prestoctl_xdr.o:	prestoctl_xdr.c
prestoctl_proc.o:	prestoctl_proc.c
prestoctl_svc.o:	prestoctl_svc.c

install:
	$(INSTALL) -c -m 711 -g system -s presto $(DESTROOT)/usr/etc/presto
	$(INSTALL) -c -m 711 -g system -s prestoctl_svc \
		$(DESTROOT)/usr/etc/prestoctl_svc

include $(GMAKERULES)

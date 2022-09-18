#  @(#)Makelocal.mk	4.2  ULTRIX  2/28/91

include $(GMAKEVARS)

OBJS = auth_none.o auth_unix.o authunix_prot.o clnt_perror.o \
	clnt_raw.o clnt_simple.o clnt_tcp.o clnt_udp.o \
	pmap_clnt.o pmap_getmaps.o pmap_getport.o pmap_prot.o \
	pmap_rmt.o rpcdtablesize.o rpc_prot.o svc.o svc_auth.o \
	svc_auth_unix.o svc_raw.o svc_simple.o svc_tcp.o svc_udp.o \
	xdr.o xdr_array.o xdr_float.o xdr_mem.o xdr_rec.o \
	xdr_reference.o xdr_stdio.o

auth_none.o:		auth_none.c
auth_unix.o:		auth_unix.c
authunix_prot.o:	authunix_prot.c
clnt_perror.o:		clnt_perror.c
clnt_raw.o:		clnt_raw.c
clnt_simple.o:		clnt_simple.c
clnt_tcp.o:		clnt_tcp.c
clnt_udp.o:		clnt_udp.c
pmap_clnt.o:		pmap_clnt.c
pmap_getmaps.o:		pmap_getmaps.c
pmap_getport.o:		pmap_getport.c
pmap_prot.o:		pmap_prot.c
pmap_rmt.o:		pmap_rmt.c
rpc_prot.o:		rpc_prot.c
rpcdtablesize.o:	rpcdtablesize.c
svc.o:			svc.c
svc_auth.o:		svc_auth.c
svc_auth_unix.o:	svc_auth_unix.c
svc_raw.o:		svc_raw.c
svc_simple.o:		svc_simple.c
svc_tcp.o:		svc_tcp.c
svc_udp.o:		svc_udp.c
xdr.o:			xdr.c
xdr_array.o:		xdr_array.c
xdr_float.o:		xdr_float.c
xdr_mem.o:		xdr_mem.c
xdr_rec.o:		xdr_rec.c
xdr_reference.o:	xdr_reference.c
xdr_stdio.o:		xdr_stdio.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	rpc.bootparamd

OBJS=	bootparam_svc.o bootparam_lib.o bootparam_xdr.o bootparam_subr.o

bootparam_svc.o:	bootparam_svc.c
bootparam_lib.o:	bootparam_lib.c
bootparam_xdr.o:	bootparam_xdr.c
bootparam_subr.o:	bootparam_subr.c

install:
	$(INSTALL) -c -m 755 -s rpc.bootparamd $(DESTROOT)/usr/etc/rpc.bootparamd

include $(GMAKERULES)

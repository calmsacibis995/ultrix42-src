#  @(#)Makelocal.mk	4.4  ULTRIX  12/20/90

include $(GMAKEVARS)

OBJS=	bindresvport.o ether_addr.o innetgr.o getcommon.o \
	gethostent.o gethostent_local.o getnetent.o getnetent_local.o \
	getnetgrent.o getprotoent.o getrpcent.o getservent.o \
	getservent_local.o herror.o hesiod.o pfilt.o rcmd.o \
	res_auth.o res_comp.o res_debug.o res_init.o res_mkquery.o \
	res_query.o res_send.o resolve.o rexec.o ruserpass.o strcasecmp.o 

bindresvport.o:		bindresvport.c
ether_addr.o:		ether_addr.c
innetgr.o:		innetgr.c
getcommon.o:		getcommon.c
gethostent.o:		gethostent.c
gethostent_local.o:	gethostent_local.c
getnetent.o:		getnetent.c
getnetent_local.o:	getnetent_local.c
getnetgrent.o:		getnetgrent.c
getprotoent.o:		getprotoent.c
getrpcent.o:		getrpcent.c
getservent.o:		getservent.c
getservent_local.o:	getservent_local.c
herror.o:		herror.c
hesiod.o:		hesiod.c
pfilt.o:		pfilt.c
rcmd.o:			rcmd.c
res_auth.o:		res_auth.c
res_comp.o:		res_comp.c
res_debug.o:		res_debug.c
res_init.o:		res_init.c
res_mkquery.o:		res_mkquery.c
res_query.o:		res_query.c
res_send.o:		res_send.c
resolve.o:		resolve.c
rexec.o:		rexec.c
ruserpass.o:		ruserpass.c
strcasecmp.o:		strcasecmp.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)

#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	mountxdr.o rquotaxdr.o rstatxdr.o rusersxdr.o util.o \
	sm_inter.o yp_all.o yp_bind.o yp_enum.o yp_master.o \
	yp_match.o yp_order.o yperr_string.o ypprot_err.o \
	ypv1_xdr.o ypxdr.o yppasswdxdr.o netname.o

mountxdr.o:		mountxdr.c
rquotaxdr.o:		rquotaxdr.c
rstatxdr.o:		rstatxdr.c
rusersxdr.o:		rusersxdr.c
util.o:			util.c
sm_inter.o:		sm_inter.c
yp_all.o:		yp_all.c
yp_bind.o:		yp_bind.c
yp_enum.o:		yp_enum.c
yp_master.o:		yp_master.c
yp_match.o:		yp_match.c
yp_order.o:		yp_order.c
yperr_string.o:		yperr_string.c
ypprot_err.o:		ypprot_err.c
ypv1_xdr.o:		ypv1_xdr.c
ypxdr.o:		ypxdr.c
yppasswdxdr.o:		yppasswdxdr.c
netname.o:		netname.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)

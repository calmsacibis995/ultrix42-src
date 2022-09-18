#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	inet_addr.o inet_network.o inet_netof.o inet_ntoa.o \
	inet_lnaof.o inet_makeaddr.o

inet_addr.o:		inet_addr.c
inet_network.o:		inet_network.c
inet_netof.o:		inet_netof.c
inet_ntoa.o:		inet_ntoa.c
inet_lnaof.o:		inet_lnaof.c
inet_makeaddr.o:	inet_makeaddr.c

include ../Makelocal_$(MACHINE).mk

all:	$(OBJS)

include $(GMAKERULES)

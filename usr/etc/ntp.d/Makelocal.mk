# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

include ../Makelocal_$(MACHINE).mk

#
# Object files
#
OBJS=   ntp.o \
        ntpd.o \
        ntpdc.o \
        ntpsubs.o \
        ntp_sock.o \
        ntp_adjust.o \
	ntp_proto.o \
	read_local.o 

AOUTS=	ntp ntpd ntpdc ntest

ntp:    ntp.o ntpsubs.o

ntpd:   ntpd.o ntpsubs.o ntp_proto.o ntp_sock.o ntp_adjust.o read_local.o 

ntpdc: ntpdc.o 

ntest: test.o ntpsubs.o

ntp.o:		ntp.c
ntpd.o:		ntpd.c
ntpdc.o:	ntpdc.c
ntpsubs.o:	ntpsubs.c
ntp_sock.o:	ntp_sock.c
ntp_adjust.o:	ntp_adjust.c
ntp_proto.o:    ntp_proto.c
read_local.o:   read_local.c
test.o:		test.c

install: ntpd ntp ntpdc
                                                                    
	$(INSTALL) -c -s -o bin -g bin -m 744 ntpd \
		$(DESTROOT)/usr/etc/ntpd
	$(INSTALL) -c -s -o bin -g bin -m 755 ntp \
		$(DESTROOT)/usr/etc/ntp
	$(INSTALL) -c -s -o bin -g bin -m 755 ntpdc \
		$(DESTROOT)/usr/etc/ntpdc

include $(GMAKERULES)

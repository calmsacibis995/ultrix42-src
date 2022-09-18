# SCCSID: @(#)Makelocal.mk	4.1	ULTRIX	1/25/91
# Makefile for nfswatch.
#
# David A. Curry
# SRI International
# 333 Ravenswood Avenue
# Menlo Park, CA 94025
# davy@itstd.sri.com
#

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/etc

AOUTS=	nfswatch nfslogsum
CINCLUDES= -I. -I..
LOADLIBES= -lcurses -ltermcap
CDEFINES=

HDRS=	externs.h nfswatch.h rpcdefs.h screen.h
SRCS=	logfile.c netaddr.c nfswatch.c nit.c pfilt.c pktfilter.c rpcfilter.c \
	rpcutil.c screen.c util.c xdr.c nfslogsum.c
OBJS=	logfile.o netaddr.o nfswatch.o nit.o pfilt.o pktfilter.o rpcfilter.o \
	rpcutil.o screen.o util.o xdr.o

logfile.o:	logfile.c nfswatch.h externs.h screen.h
netaddr.o:	netaddr.c nfswatch.h externs.h
nfslogsum.o:	nfslogsum.c nfswatch.h
nfswatch.o:	nfswatch.c nfswatch.h
nit.o:		nit.c nfswatch.h externs.h
pfilt.o:	pfilt.c nfswatch.h externs.h
pktfilter.o:	pktfilter.c nfswatch.h externs.h
rpcfilter.o:	rpcfilter.c nfswatch.h externs.h rpcdefs.h
rpcutil.o:	rpcutil.c nfswatch.h externs.h rpcdefs.h
screen.o:	screen.c nfswatch.h externs.h screen.h
util.o:		util.c nfswatch.h externs.h screen.h
xdr.o:		xdr.c nfswatch.h
nfswatch:	${OBJS}
nfslogsum:	nfslogsum.o

all:	nfswatch nfslogsum
	
install:
	$(INSTALL) -s -c nfswatch $(DESTROOT)/usr/etc/nfswatch
	$(INSTALL) -s -c nfslogsum $(DESTROOT)/usr/etc/nfslogsum

include $(GMAKERULES)

# @(#)Makelocal.mk	4.1   ULTRIX  7/17/90
#
# make file for tip device drivers
#
# Current drivers:
#       BIZCOMP
#       DEC DF02-AC, DF03-AC
#       DEC DN-11/Able Quadracall
#       VENTEL 212+ (w/o echo)
#       VADIC 831 RS232 adaptor
#       VADIC 3451
#       DEC DF112
#       Generic (a way of handling other modems)
include $(GMAKEVARS)

CFLAGS= -O
CDEFINES= -DGENACU -DONDELAY
CINCLUDES= -I. -I.. -I../.. -I$(SRCROOT)/usr/include
OBJS=   remcap.o biz22.o biz31.o df.o dn11.o ventel.o v831.o v3451.o df112.o generic.o gen.o
ARFLAGS=cu
ARFILE=aculib.a

remcap.o:	remcap.c
biz22.o:	biz22.c
biz31.o:	biz31.c
df.o:	df.c
dn11.o:	dn11.c
ventel.o:	ventel.c
v831.o:	v831.c
v3451.o:	v3451.c
df112.o:	df112.c
generic.o:	generic.c
gen.o:	gen.c

${OBJS}: ../../tip.h

install:

include $(GMAKERULES)

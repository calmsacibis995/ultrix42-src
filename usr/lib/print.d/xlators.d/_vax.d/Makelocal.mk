# @(#)Makelocal.mk	4.1      ULTRIX 	7/2/90
# Makefile for lpr.d/xlators.d/regis_tek.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989
#
# Modification History
#
# 10-jan-90 - Stuart Murdoch
#	Added -V lk_object flag to LDCMD to force use of lk

include $(GMAKEVARS)

LDCMD =		$(CC) -V LKOBJECT $(LDFLAGS) -o $@
CC=vcc
XLDIR		=usr/lib/lpdfilters

DESTLIST	=$(DESTROOT)/$(XLDIR)

CINCLUDES =	-I. -I.. -I../../../h.d -I$(SRCROOT)/usr/include

LIBLP		= ../../../lib.d/_$(MACHINE).b/liblp.a

LIBS		= /usr/lib/fortrtl.a $(LIBLP)

OBJS		= ultrix_main.o ultrix_io.o call_stubs.o

COMMON_OBJS	= ultrix_main.o ultrix_io.o call_stubs.o \
		../mth_floor.uob

CFILES		= ultrix_main.c ultrix_io.c call_stubs.c

REGISOBJS	= interface.uob gidis.uob \
		col222.uob mgmm.uob regdata.uob regis.uob \
		regis1.uob regis2.uob regis3.uob regis4.uob scan.uob


TEK4014OBJS	= interface.uob gidis.uob \
		tekparser.uob tekdata.uob 4014.uob

all:		myall

myall:		regis_ps tek4014_ps

regis_ps:	$(COMMON_OBJS) $(REGISOBJS)
		cd ..; ln $(REGISOBJS) _vax.b
		${LDCMD} $(COMMON_OBJS) $(REGISOBJS) $(LIBS)
		$(RM) $(REGISOBJS)

tek4014_ps:	$(COMMON_OBJS) $(TEK4014OBJS)
		cd ..; ln $(TEK4014OBJS) _vax.b
		${LDCMD} $(COMMON_OBJS) $(TEK4014OBJS) $(LIBS)
		$(RM) $(TEK4014OBJS)

install:
		install -c -s regis_ps $(DESTROOT)/$(XLDIR)/regis_ps
		install -c -s tek4014_ps $(DESTROOT)/$(XLDIR)/tek4014_ps


call_stubs.o: call_stubs.s
	$(AS) -o $@ ../call_stubs.s

ultrix_main.o: ultrix_main.c
ultrix_io.o: ultrix_io.c

include $(GMAKERULES)

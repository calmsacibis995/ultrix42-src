# @(#)Makelocal.mk	4.4      LPS_ULT 	11/15/90
#
# ****************************************************************************
# *									     *
# *  COPYRIGHT (c) 1987, 1988, 1989, 1990				     *
# *  By DIGITAL EQUIPMENT CORPORATION, Maynard, Mass.			     *
# *									     *
# *  This software is furnished under a license and may be used and  copied  *
# *  only  in  accordance  with  the  terms  of  such  license and with the  *
# *  inclusion of the above copyright notice.  This software or  any  other  *
# *  copies  thereof may not be provided or otherwise made available to an   *
# *  other person.  No title to and ownership of  the  software  is  hereby  *
# *  transferred.      							     *
# *									     *
# *  The information in this software is subject to change  without  notice  *
# *  and  should  not  be  construed  as  a commitment by Digital Equipment  *
# *  Corporation.							     *
# *									     *
# *  Digital assumes no responsibility for the use or  reliability  of  its  *
# *   software on equipment which is not supplied by Digital.		     *
# *									     *
# ****************************************************************************
#
# Makefile to build (print.d/filters.d) LPSCOMM and IPLPSCOMM
#
# V0.008  AT  13-Nov-1990 Add new files to iplpscomm
# V0.007  AT  01-Oct-1990 Install iplps error number file in /usr/example/print
# V0.006  AKR 10-Apr-1989 Just use generic make rules
# V0.005  APK  7-Apr-1989 Make it compatible with Ultrix-multi ver. build
# V0.004  APK  7-Apr-1989 Add iplpscomm build
# V0.003  APK 13-Oct-1988 Add dependency for libdnet changed in ULTRIX V3.0
# V0.002  APK 26-Jul-1988 Add dependency for header files
# V0.001  APK 29-Jun-1988 change 'laps' to 'lpscomm'
#

include $(GMAKEVARS)

XLDIR=usr/lib/lpdfilters
EXDIR=usr/examples/print

DESTLIST=\
	$(DESTROOT)/etc \
	$(DESTROOT)/usr \
	$(DESTROOT)/usr/lib \
	$(DESTROOT)/usr/examples \
	$(DESTROOT)/$(EXDIR) \
	$(DESTROOT)/$(XLDIR)

LIBS=-ldnet

LPOBJS=laps.o main.o net_ultrix.o laps_msg.o

IPOBJS=iplpscomm.o lpcomm.o ultrix_utilities.o ultrix_internet.o

HFILES=laps.h lpsmsg_string.h lpsmsg.h descrip.h lps.h \
	ultrix_utilities.h paprelay.h

CFILES=laps.c main.c net_ultrix.c laps_msg.c iplpscomm.c lpcomm.c

all:	myall

myall:	lpscomm iplpscomm

lpscomm:	${LPOBJS}
	${LDCMD} ${LPOBJS} ${LIBS}

iplpscomm: ${IPOBJS}
	${LDCMD} ${IPOBJS}

install:  
	install -c -s lpscomm $(DESTROOT)/$(XLDIR)/lpscomm
	install -c -s iplpscomm $(DESTROOT)/$(XLDIR)/iplpscomm
	install -c -m 444 ../iplps_errors $(DESTROOT)/$(EXDIR)/iplps_errors


${LPOBJS}:      laps.h lpsmsg.h descrip.h lpsmsg_string.h

${IPOBJS}:      lps.h paprelay.h ultrix_utilities.h

laps.o: laps.c
main.o: main.c
net_ultrix.o: net_ultrix.c
laps_msg.o: laps_msg.c
iplpscomm.o: iplpscomm.c
lpcomm.o: lpcomm.c
ultrix_utilities.o: ultrix_utilities.c
ultrix_internet.o: ultrix_internet.c

include $(GMAKERULES)

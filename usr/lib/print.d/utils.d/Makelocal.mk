# @(#)Makelocal.mk	4.1      ULTRIX	7/2/90

# Makefile for lpr.d/utils.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		11th May 1989
#

include $(GMAKEVARS)

LIBDIR=usr/lib
BINDIR=usr/ucb
SPOOLDIR=usr/spool/lpd

DAEMON=daemon
SPGRP=daemon

LIBLP=../../lib.d/_$(MACHINE).b/liblp.a

CINCLUDES=-I. -I.. -I../../h.d -I$(SRCROOT)/usr/include

DESTLIST= $(DESTROOT)/usr/etc $(DESTROOT)/$(LIBDIR) $(DESTROOT)/etc \
	$(DESTROOT)/$(BINDIR) $(DESTROOT)/$(SPOOLDIR)

all:	lpd lpserrof lpr lpq lprm pac lpc

LPD_OBJS=lpd.o logging.o printjob.o pcap_choices.o recvjob.o \
	sendjob.o displayq.o rmjob.o dobanner.o print_utils.o \
	startdaemon.o lpdchar.o common.o printcap.o lat_conn.o tcp_conn.o \
	filter.o escapes.o dcl.o dcl_xar.o dm_handler.o connection.o \
	startdqs.o getdqport.o monitor.o

LPROBJS=lpr.o startdaemon.o printcap.o
LPQOBJS=lpq.o displayq.o common.o printcap.o getdqport.o logging.o
LPRMOBJS=lprm.o rmjob.o startdaemon.o common.o printcap.o getdqport.o logging.o
LPCOBJS=lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o
LPSERROF=lpserrof.o filter.o logging.o

lpd:	${LPD_OBJS}
	${CC} -o lpd ${LPD_OBJS} ${LIBLP}

lpserrof: $(LPSERROF)
	${CC} -o lpserrof $(LPSERROF) ${LIBLP}

lpr:	$(LPROBJS) ${LIBLP}
	${CC} -o lpr $(LPROBJS) ${LIBLP}

lpq:	$(LPQOBJS)
	${CC} -o lpq $(LPQOBJS) -ltermcap

lprm:	$(LPRMOBJS)
	${CC} -o lprm $(LPRMOBJS)

lpc:	$(LPCOBJS)
	${CC} -o lpc $(LPCOBJS)

pac:	pac.o printcap.o
	${LDCMD} pac.o printcap.o

#	parse_prog is a debugging utility to test the
#	filter.c module which handles all sub-process invocation

parse_prog:	parse_prog.o filter.o escapes.o logging.o
	${LDCMD}_prog parse_prog.o filter.o escapes.o logging.o ${LIBLP}

install:
	$(INSTALL) -c -s -g system -m 700 lpd ${DESTROOT}/${LIBDIR}
	$(INSTALL) -c -s -g system -m 711 lpserrof ${DESTROOT}/${LIBDIR}
	$(INSTALL) -c -s -g ${SPGRP} -m 6711 lpr ${DESTROOT}/${BINDIR}
	$(INSTALL) -c -s -g ${SPGRP} -m 6711 lpq ${DESTROOT}/${BINDIR}
	$(INSTALL) -c -s -g ${SPGRP} -m 6711 lprm ${DESTROOT}/${BINDIR}
	$(INSTALL) -c -s -g ${SPGRP} -m 2711 lpc ${DESTROOT}/usr/etc
	$(INSTALL) -c -s pac ${DESTROOT}/etc

	$(CHOWN) ${DAEMON} ${DESTROOT}/${SPOOLDIR}
	$(CHGRP) ${SPGRP} ${DESTROOT}/${SPOOLDIR}
	$(CHMOD) 775 ${DESTROOT}/${SPOOLDIR}

	$(RM) ${DESTROOT}/etc/lpc
	$(LN) -s ../usr/etc/lpc ${DESTROOT}/etc/lpc

lpd.o: lpd.c
lpr.o: lpr.c
lpq.o: lpq.c
lprm.o: lprm.c
pac.o: pac.c
lpc.o: lpc.c
lpserrof.o: lpserrof.c
cmds.o: cmds.c
cmdtab.o: cmdtab.c
logging.o: logging.c
printjob.o: printjob.c
pcap_choices.o: pcap_choices.c
recvjob.o: recvjob.c
sendjob.o: sendjob.c
displayq.o: displayq.c
rmjob.o: rmjob.c
dobanner.o: dobanner.c
print_utils.o: print_utils.c
startdaemon.o: startdaemon.c
lpdchar.o: lpdchar.c
common.o: common.c
printcap.o: printcap.c
lat_conn.o: lat_conn.c
tcp_conn.o: tcp_conn.c
filter.o: filter.c
escapes.o: escapes.c
dcl.o: dcl.c
dcl_xar.o: dcl_xar.c
dm_handler.o: dm_handler.c
connection.o: connection.c
startdqs.o: startdqs.c
getdqport.o: getdqport.c
monitor.o: monitor.c

${LPD_OBJS}:	lp.h lp.local.h

printjob.o: escapes.h filter.h dcl.h
dm_handler.o dcl.o dcl_xar.o: filter.h dcl.h

print_utils.o filter.o:	filter.h
escapes.o:	escapes.h filter.h
connection.o:	connection.h
escapes.o filter.o: lp.h lp.local.h
startdqs.o: lp.h
getdqport: lp.h
startdaemon.o: lp.local.h
lpr.o lpq.o lprm.o pac.o: lp.h lp.local.h
lpc.o cmdtab.o: lpc.h
cmds.o: lp.h lp.local.h

include $(GMAKERULES)

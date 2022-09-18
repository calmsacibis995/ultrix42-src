#  @(#)makefile.ex	4.1  ULTRIX  7/17/90
#
#  Makefile to build an example snmpextd daemon.
#

DESTDIR= /

OBJS=	main.o disk_grp.o snmp.o vm_grp.o

SRCS=	main.c disk_grp.c snmp.c vm_grp.c

CFLAGS= -O
LIBS=	-lsnmp

.c.o:
	cc -c ${CFLAGS} $*.c

snmpextd: ${OBJS}
	cc ${OBJS} ${CFLAGS} -o snmpextd ${LIBS}

clean:
	-rm -f snmpd *.o core linterrs

install: snmpextd
	install -c -s snmpextd ${DESTDIR}/etc
	@echo "Don't forget to add snmpextd into /etc/snmpd.conf file"


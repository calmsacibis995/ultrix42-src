# SCCSID: @(#)Makelocal.mk	4.2	ULTRIX	1/25/91

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/etc

AOUTS=	tcpdump
CINCLUDES= -I. -I..
YFLAGS= -d
LOADLIBES= -ll
CDEFINES= -DDEBUG -DULTRIX -DPACKETFILTER

CSRC= addrtoname.c bpf_dump.c bpf_filter.c bpf_image.c etherent.c \
	gencode.c inet.c md.c nametoaddr.c optimize.c os-ultrix.c \
	pcap-pf.c print-arp.c print-atalk.c print-bootp.c \
	print-domain.c print-ether.c print-icmp.c print-ip.c print-nfs.c \
	print-ntp.c print-ppp.c print-rip.c print-sl.c print-snmp.c \
	print-tcp.c print-tftp.c print-udp.c savefile.c \
	tcpdump.c util.c

SRC=	$(CSRC) tcpgram.y tcplex.l

OBJ= addrtoname.o bpf_dump.o bpf_filter.o bpf_image.o etherent.o \
	gencode.o inet.o md.o nametoaddr.o optimize.o os-ultrix.o \
	pcap-pf.o print-arp.o print-atalk.o print-bootp.o \
	print-domain.o print-ether.o print-icmp.o print-ip.o print-nfs.o \
	print-ntp.o print-ppp.o print-rip.o print-sl.o print-snmp.o \
	print-tcp.o print-tftp.o print-udp.o savefile.o \
	tcpdump.o tcpgram.o tcplex.o util.o

HDR= addrtoname.h appletalk.h bootp.h etherent.h etherproto.h \
	extract.h gencode.h interface.h md.h \
	mib.h nametoaddr.h ntp.h os.h savefile.h

AWKS=	atime.awk packetdat.awk send-ack.awk stime.awk

addrtoname.o:	addrtoname.c addrtoname.h
bpf_dump.o:	bpf_dump.c
bpf_filter.o:	bpf_filter.c
bpf_image.o:	bpf_image.c
etherent.o:	etherent.c etherent.h
gencode.o:	gencode.c gencode.h
inet.o:		inet.c
md.o:		md.c md.h
nametoaddr.o:	nametoaddr.c nametoaddr.h
os-ultrix.o:	os-ultrix.c os.h
pcap-pf.o:	pcap-pf.c
print-arp.o:	print-arp.c
print-atalk.o:	print-atalk.c
print-bootp.o:	print-bootp.c
print-domain.o:	print-domain.c
print-ether.o:	print-ether.c
print-icmp.o:	print-icmp.c
print-ip.o:	print-ip.c
print-nfs.o:	print-nfs.c
print-ntp.o:	print-ntp.c
print-ppp.o:	print-ppp.c
print-rip.o:	print-rip.c
print-sl.o:	print-sl.c
print-snmp.o:	print-snmp.c
print-tcp.o:	print-tcp.c
print-tftp.o:	print-tftp.c
print-udp.o:	print-udp.c
savefile.o:	savefile.c savefile.h
util.o:		util.c
tcpdump.o:	tcpdump.c
optimize.o:	optimize.c
tcpgram.o:	tcpgram.y
tcplex.o:	tcplex.l tokdefs.h
tcpdump:	${OBJ}

all:	tcpdump
	
tokdefs.h: tcpgram.o
	-mv y.tab.h tokdefs.h

install:
	$(INSTALL) -s -c tcpdump $(DESTROOT)/usr/etc/tcpdump

include $(GMAKERULES)

#
#     @(#)Makefile	4.4     (ULTRIX)        3/6/91
#
# Normal sequence of events is: clean clobber sccsget all install
#

CLOBBERDIR= net netbsc netimp netinet netdnet lat dli rpc \
	rpcsvc

TARDIRS= net netbsc netimp netinet netdnet lat dli rpc rpcsvc

DESTROOT=
DESTDIR=${DESTROOT}/usr/sys/net

#
# Files in the netinet and netbsc dirs that go on the kit.
#
#
NETINET-FILES=icmp_var.h in_proto.c ip_var.h tcp_seq.h udp.h if_ether.h \
	if_fddi.h in_systm.h tcp.h tcp_timer.h udp_var.h in.h ip.h \
	tcp_debug.h tcp_var.h in_pcb.h ip_icmp.h tcp_fsm.h tcpip.h in_var.h

NETDNET-FILES=	\
	dli_var.h dn.h decnet_types.h nsp_addr.h \
	node_params.h evl.h nm_defines.h dnetdb.h decnet_dummy.c

NETDNET-HDR-FILES=	\
	nsp_protocol.h nsp_timer.h nsp_nm.h nsp_var.h

NETLAT-FILES=	lat.h

NETBSC-FILES=bsc.h bsc_messages.h bsc_proto.c bsc_proto.h \
	bsc_states.h bsc_timer.h bsc_var.h

NET-FILES= *.h conf_net.c

DLI-FILES= dli_proto.c

all:

clean:

clobber: clean
	for i in ${CLOBBERDIR}; do \
	cd $$i;sccs clean;cd ..;done

sccsinfo:
	@for i in ${TARDIRS} ; do cd $$i;echo -n "$$i:";sccs info;cd .. ; done

sccsget:
	@-for i in ${TARDIRS};do cd $$i;echo "$$i";sccs get -s SCCS;cd ..;done

install: all instdirs inst-netbsc inst-net inst-netimp inst-netinet \
	inst-netdnet inst-rpc inst-rpcsvc inst-dli inst-lat

insthdrs:
	@echo Installing netdent header files needed for build environment.
	@cd netdnet;tar cFf - ${NETDNET-HDR-FILES}|(cd ${DESTDIR}/netdnet;tar xpf -; chmod ugo+r *)

instdirs:
	-if [ ! -d ${DESTDIR} ] ;\
	then \
		mkdir ${DESTDIR}; \
		/etc/chown root ${DESTDIR}; \
		chgrp system ${DESTDIR}; \
		chmod 0755 ${DESTDIR}; \
	else true; \
	fi
	@rm -f ${DESTROOT}/sys
	@cd ${DESTROOT}; ln -s usr/sys ${DESTROOT}/sys
	@-for i in  ${TARDIRS} ; do \
		if [ ! -d ${DESTDIR}/$$i ] ;\
		then \
			mkdir ${DESTDIR}/$$i; \
			/etc/chown root ${DESTDIR}/$$i; \
			chgrp system ${DESTDIR}/$$i; \
			chmod 0755 ${DESTDIR}/$$i; \
		else true; \
		fi; \
	done

inst-netbsc:
	@echo Installing netbsc files.
	@cd netbsc;tar cFf - ${NETBSC-FILES}|( cd ${DESTDIR}/netbsc; tar xpf - ; chmod ugo+r *)

inst-net:
	@echo Installing net files.
	@cd net;tar cFf - ${NET-FILES}|(cd ${DESTDIR}/net;tar xpf - ; chmod ugo+r * )

inst-netimp:
	@echo Installing netimp files.
	@cd netimp;tar cFf - *.h|(cd ${DESTDIR}/netimp;tar xpf -;chmod ugo+r *)

inst-netinet:
	@echo Installing netinet files.
	@cd netinet;tar cFf - ${NETINET-FILES}|( cd ${DESTDIR}/netinet; tar xpf - ; chmod ugo+r *)

inst-netdnet:
	@echo Installing netdnet files.
	@cd netdnet;tar cFf - ${NETDNET-FILES}|(cd ${DESTDIR}/netdnet;tar xpf -; chmod ugo+r *)

inst-lat:
	@echo Installing lat files.
	@cd lat;tar cFf - ${NETLAT-FILES}|(cd ${DESTDIR}/lat;tar xpf -; chmod ugo+r *)

inst-rpc:
	@echo Installing rpc files.
	@cd rpc;tar cFf - *.h|(cd ${DESTDIR}/rpc;tar xpf -; chmod ugo+r *)

inst-rpcsvc:
	@echo Installing rpcsvc files.
	@cd rpcsvc;tar cFf - *.h|(cd ${DESTDIR}/rpcsvc;tar xpf -; chmod ugo+r *)

inst-dli:
	@echo Installing dli files.
	@cd dli;tar cFf - ${DLI-FILES}|(cd ${DESTDIR}/dli;tar xpf - ; chmod ugo+r * )

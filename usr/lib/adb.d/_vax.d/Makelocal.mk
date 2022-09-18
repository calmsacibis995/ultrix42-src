#  @(#)Makelocal.mk	1.1  ULTRIX  3/20/89

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/adb

FILES = buf callout callout.next clist clist.nxt dino dir \
	dir.nxt dirblk file file.nxt filsys findproc findproc.nxt \
	host.nxt hosts hosts.nxt hosttable ifnet ifuba imp inode \
	inpcb iovec ipreass ipreass.nxt mact mact.nxt cabase classb \
	connb reqb rspid_tbl rspid_tbl.nxt unitb

FILES2 =	mbstat mbuf mbuf.nxt mbufs mbufs.nxt mount nl nu oimp \
	pcb proc protosw rawcb rtentry rusage setproc setproc.done \
	setproc.nxt socket stacktrace stat tcpcb tcpip tcpreass \
	tcpreass.nxt text traceall traceall.nxt tty u u.all \
	ubahd ucred uio un vtimes

install:
	@for i in $(FILES) $(FILES2); do \
		$(ECHO) "$(INSTALL) -c ../$$i $(DESTROOT)/usr/lib/adb/$$i"; \
		$(INSTALL) -c ../$$i $(DESTROOT)/usr/lib/adb/$$i; \
	done

include $(GMAKERULES)

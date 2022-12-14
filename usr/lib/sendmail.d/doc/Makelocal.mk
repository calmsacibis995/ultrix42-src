#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

LPRS=	intro.lpr op.lpr usenix.lpr
DOCS=	intro.me  op.me  usenix.me
SRCS=	$(DOCS) sendmail.8 syslog.3 syslog.8
ALL=	$(LPRS) rfc819.lpr rfc821.lpr rfc822.lpr

LPR=	lpr
TROFF=	vtroff

.SUFFIXES: .me .lpr

.me.lpr:
	nroff -me -Tlp $< > $@

troff: $(DOCS)
	for i in $(DOCS); do $(TROFF) -me $$i; done

print: $(ALL)
	$(LPR) $(ALL)

include $(GMAKERULES)

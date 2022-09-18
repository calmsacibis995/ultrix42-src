#	Makelocal.mk
#		sys/dist Makefile
#
#	000	27-jul-1990	ccb
#	New.
#
#	001	05-aug-1990	overman
#	moved install to /usr/etc
#
#	SCCSID	"@(#)Makelocal.mk	1.2 (ULTRIX)	8/5/90"

include $(GMAKEVARS)

ETCDIR=$(DESTROOT)/usr/etc
SCAMPDIR= $(ETCDIR)/scampdir
TEXT_INFO=$(SCAMPDIR)/text_info

DESTLIST= $(ETCDIR) $(SCAMPDIR) $(TEXT_INFO)

INSTALLFILES= lmf.txt modem.txt print.txt term.txt welcome.txt \
		network.txt

install:	$(INSTALLFILES)
	@for i in $(INSTALLFILES); \
        do \
                $(ECHO) "$(INSTALL) -c -m 755 ../$$i $(TEXT_INFO)/$$i"; \
                $(INSTALL) -c -m 755 ../$$i $(TEXT_INFO)/$$i; \
        done

include $(GMAKERULES)

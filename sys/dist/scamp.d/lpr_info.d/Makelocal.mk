#	Makelocal.mk
#		sys/dist Makefile
#
#	000	27-jul-1990	ccb
#	New.
#
#	001	05-aug-1990	overman
#	changed to install into /usr/etc
#
#	SCCSID	"@(#)Makelocal.mk	1.2 (ULTRIX)	8/5/90"

include $(GMAKEVARS)

ETCDIR=$(DESTROOT)/usr/etc
SCAMPDIR= $(ETCDIR)/scampdir
LPR_INFO=$(SCAMPDIR)/lpr_info

DESTLIST= $(ETCDIR) $(SCAMPDIR) $(LPR_INFO)

INSTALLFILES= LA210 LA75 LN03 LN03R LN03S

install:	$(INSTALLFILES)
	@for i in $(INSTALLFILES); \
        do \
                $(ECHO) "$(INSTALL) -c -m 755 ../$$i $(LPR_INFO)/$$i"; \
                $(INSTALL) -c -m 755 ../$$i $(LPR_INFO)/$$i; \
        done

include $(GMAKERULES)

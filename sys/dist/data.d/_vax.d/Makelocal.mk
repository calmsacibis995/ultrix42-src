#  "@(#)Makelocal.mk	4.4 (ULTRIX) 12/20/90"
#
#	001	16-Oct-90	overman
#	Added V4.2 to mi an k
#
#	000	17-jul-1990	mdf
#	Updated from V4.0 to V4.1 
#

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

KFILES=ULT420.k ULT421.k ULX420.k ULC420.k
MIFILES=ULT420.mi ULT421.mi

install:
	@for i in $(KFILES) $(MIFILES); \
	do \
		echo "$(INSTALL) -c -m 644 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)

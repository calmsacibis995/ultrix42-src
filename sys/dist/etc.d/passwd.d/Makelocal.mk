#	Makelocal.mk -
#		sys/dist/etc.d/passwd.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc $(DESTROOT)/usr/etc

SCRIPTS= adduser lockpw removeuser unlockpw

install:
	@for i in $(SCRIPTS); \
	do \
		echo "$(INSTALL) -c -m 754 ../$$i.sh $(DESTROOT)/usr/etc/$$i"; \
		$(INSTALL) -c -m 754 ../$$i.sh $(DESTROOT)/usr/etc/$$i; \
		echo "$(RM) $(DESTROOT)/etc/$$i"; \
		$(RM) $(DESTROOT)/etc/$$i; \
		echo "$(LN) -s ../usr/etc/$$i $(DESTROOT)/etc/$$i"; \
		$(LN) -s ../usr/etc/$$i $(DESTROOT)/etc/$$i; \
	done
	$(INSTALL) -c -o root -m 644 ../passwd $(DESTROOT)/etc/passwd

include $(GMAKERULES)

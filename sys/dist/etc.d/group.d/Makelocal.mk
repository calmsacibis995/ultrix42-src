#	Makelocal.mk -
#		sys/dist/etc.d/group Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New. Installs /etc/group and group management software

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc $(DESTROOT)/usr/etc

install:
	$(INSTALL) -c -m 644 ../group $(DESTROOT)/etc/group
	$(INSTALL) -c -m 754 ../addgroup.sh $(DESTROOT)/usr/etc/addgroup
	-($(RM) $(DESTROOT)/etc/addgroup; true)
	$(LN) -s ../usr/etc/addgroup $(DESTROOT)/etc/addgroup

include $(GMAKERULES)

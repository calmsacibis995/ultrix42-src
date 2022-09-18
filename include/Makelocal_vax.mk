#	@(#)Makelocal_vax.mk	4.1	(ULTRIX)	7/2/90
SYSDIRS=${NETDIRS} ${FSDIRS} ${TOPDIRS} sas
SUBDIRECTS=arpa pascal protocols

#
# Machine specific links
#
machlinks:
#
#	These links are for backwards compatibility with previous versions
#
	rm -rf ${DESTROOT}/usr/include/vaxif
	ln -s /sys/io/netif ${DESTROOT}/usr/include/vaxif
	rm -rf ${DESTROOT}/usr/include/vaxmba
	ln -s /sys/io/mba/vax ${DESTROOT}/usr/include/vaxmba
	rm -rf ${DESTROOT}/usr/include/vaxsysap
	ln -s /sys/io/sysap ${DESTROOT}/usr/include/vaxsysap
	rm -rf ${DESTROOT}/usr/include/vaxuba
	ln -s /sys/io/uba ${DESTROOT}/usr/include/vaxuba
	rm -rf ${DESTROOT}/usr/include/vaxscsi
	ln -s /sys/io/scsi/vax ${DESTROOT}/usr/include/vaxscsi


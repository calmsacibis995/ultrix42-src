#  @(#)Makelocal.mk	4.3  ULTRIX  11/9/90
include $(GMAKEVARS)
#
# Doing a make install builds /usr/include
#
# Define SHARED to indicate whether you want
# symbolic links to the system source (``symlinks''),
# or a separate copy (``copies'').
# (latter useful in environments where it's
# not possible to keep /sys publicly readable)
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
STD=	a.out.h ar.h assert.h capsar.h cat.h cpio.h curses.h dbm.h des.h \
	dial.h dirent.h disktab.h dumprestor.h elcsd.h \
	elwindow.h execargs.h fatal.h fcntl.h float.h \
	fstab.h ftw.h grp.h hesiod.h i_defs.h i_errno.h imghdr.h krb.h \
	langinfo.h lastlog.h locale.h macros.h \
	malloc.h math.h memory.h misc.h mit-copyright.h mon.h mp.h mtab.h \
	nan.h ndbm.h netdb.h nl_types.h nlist.h nlm_prot.h prof.h pwd.h \
	ranlib.h regexp.h resolv.h resscan.h search.h setjmp.h stab.h \
	stand.h stdarg.h stddef.h stdio.h stdlib.h strings.h struct.h \
	sysexits.h syslog.h tar.h term.h \
	termio.h termios.h ttyent.h tzfile.h \
	ulimit.h unctrl.h unistd.h utime.h utmp.h \
	values.h varargs.h vfont.h

LINKS= limits.h ustat.h sgtty.h ctype.h errno.h signal.h syscall.h \
	time.h lmf.h xti.h ansi_compat.h

NETDIRS=net/net net/netimp net/netinet net/netdnet net/rpc \
	net/rpcsvc net/netbsc net/lat
FSDIRS=	fs/ufs fs/nfs fs/cdfs
TOPDIRS= io fs
ALLDIRS=${NETDIRS} ${FSDIRS} sas ${VAXDEP}
ALLSUBDIRS=arpa pascal protocols prom cmplrs
SHARED=	symlinks
DESTLIST= ${DESTROOT}/usr/include

include ../Makelocal_${MACHINE}.mk
# the above include sets SYSDIRS and SUBDIRS for the specific machine....
# the install only install machine specific...
# the clean, clobber, get, info does alldirs.....

all:

install: ${SHARED}
	-for i in ${STD} ${MACH-STD}; do \
		install -c -m 444 ../$$i ${DESTROOT}/usr/include/$$i; \
	done
	-for i in ${SUBDIRECTS}; do \
		if [ ! -d ${DESTROOT}/usr/include/$$i ]; then \
			mkdir ${DESTROOT}/usr/include/$$i; \
		fi; \
		(cd ../$$i; for j in *.[ih]; do \
			install -c -m 444 $$j ${DESTROOT}/usr/include/$$i/$$j; \
		done); \
	done
	-for i in ${LINKS}; do \
		rm -f ${DESTROOT}/usr/include/$$i; \
		ln -s sys/$$i ${DESTROOT}/usr/include/$$i; \
	done
	rm -f ${DESTROOT}/usr/include/string.h
	ln -s ./strings.h ${DESTROOT}/usr/include/string.h
	rm -f ${DESTROOT}/usr/include/machine
	ln -s /sys/machine/${MACHINE} ${DESTROOT}/usr/include/machine
	rm -f ${DESTROOT}/usr/include/frame.h
	ln -s machine/frame.h ${DESTROOT}/usr/include/frame.h

#
#	machlinks is defined in the machine-specific rules file
#
symlinks: $(DESTLIST) machlinks
	for i in ${SYSDIRS}; do \
		base=`basename $$i`; \
		rm -rf ${DESTROOT}/usr/include/$$base; \
		ln -s /sys/$$i ${DESTROOT}/usr/include/$$base; \
	done
	rm -rf ${DESTROOT}/usr/include/sys
	ln -s /sys/h ${DESTROOT}/usr/include/sys
	rm -rf ${DESTROOT}/usr/include/$(MACHINE)
	ln -s /sys/machine/$(MACHINE) ${DESTROOT}/usr/include/$(MACHINE)

copies: $(DESTLIST) 
	for i in ${SYSDIRS}; do \
		rm -rf ${DESTROOT}/usr/include/$$i; \
		cd /sys; \
		tar cf - $$i/*.h | (cd /usr/include; tar xpf -); \
	done
	rm -rf ${DESTROOT}/usr/include/sys;
	mkdir ${DESTROOT}/usr/include/sys; 
	chmod 775 ${DESTROOT}/usr/include/sys;
	(cd /sys/h; tar cf - *.h | (cd ${DESTROOT}/usr/include/sys; tar xpf -))


include $(GMAKERULES)

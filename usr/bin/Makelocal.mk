# @(#)Makelocal.mk	4.5      ULTRIX	1/31/91

include $(GMAKEVARS)

SUBDIRS=2780e.d file.d _$(MACHINE).d ar.d ar11.d at.d awk.d \
	basename.d batch.d bc.d cal.d calendar.d capsar.d catpw.d \
	cb.d cc.d cflow.d checkeq.d chgrp.d chroot.d cmp.d col.d comm.d cpio.d \
	cpu.d crash.d crypt.d csh.d csplit.d ctrace.d cut.d cxref.d dc.d \
	deroff.d df.d dgate.d diction.d diff.d diff3.d diffmk.d \
	dircmp.d domainname.d du.d efl.d egrep.d eqn.d env.d \
	f77.d fgrep.d find.d \
	gencat.d getopt.d graph.d hostid.d ic.d iconv.d id.d install.d \
	iostat.d ipcrm.d ipcs.d join.d ksh.d learn.d lex.d line.d lint.d \
	login.d logname.d look.d lorder.d ltf.d m4.d mail.d \
	make.d man.d mesg.d mh mhdecode.d mkcatdefs.d mkfifo.d mktemp.d \
        mmapalignment.d modula2.d neqn.d \
	newawk.d nice.d nl.d nm.d nohup.d nroff.d od.d pack.d pagesize.d \
	passwd.d paste.d pfstat.d pg.d plot.d pr.d ptoc.d ptx.d ranlib.d \
	ratfor.d refer.d rev.d rmail.d rmdir.d s5make.d sccs.d sed.d \
	sh5.d shexp.d size.d sleep.d sort.d sort5.d spell.d spline.d split.d \
	strip.d struct.d su.d sum.d tabs.d tbl.d tc.d tee.d text.d time.d \
	tip.d tk.d touch.d tp.d tr.d trace.d troff.d tsort.d tty.d \
	uname.d uniq.d units.d wall.d who.d write.d xargs.d xsend.d yacc.d \
	ypcat.d ypmatch.d yppasswd.d ypwhich.d

include $(GMAKERULES)

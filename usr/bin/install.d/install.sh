#! /usr/bin/ksh
#
#  @(#)install.sh	4.1	ULTRIX	7/17/90
#
cmd=/bin/mv
strip=""
chmod="/bin/chmod 755"
chown="/etc/chown -f root"
chgrp="/bin/chgrp -f system"
while :
do
	case $1 in
	-s )	strip="/bin/strip"
		;;
	-c )	cmd="/bin/cp"
		;;
	-m )	chmod="/bin/chmod $2"
		shift
		;;
	-o )	chown="/etc/chown -f $2"
		shift
		;;
	-g )	chgrp="/bin/chgrp -f $2"
		shift
		;;
	* )	break
		;;
	esac
	shift
done

case "$2" in
"")	print -u2 "install: no destination specified"
	exit 1
	;;
.|"$1")	print -u2 "install: can't move $1 onto itself"
	exit 1
	;;
esac
case "$3" in
"")	;;
*)	print -u2 "install: too many files specified -> $*"
	exit 1
	;;
esac
if [ -d $2 ]
then	file=$2/$1
else	file=$2
fi
if [ -r $1 ]
then
	/bin/rm -f $file
else
	print -u2 "install: file $1 does not exist."
	exit 1
fi
#
# mkdir if directory does not exist.
#
mkdir -p $(dirname $file)
	
$cmd $1 $file
[ $strip ] && $strip $file
$chown $file
$chgrp $file
$chmod $file


# @(#)man.sh	4.1	(ULTRIX)	7/17/90
cmd= sec= fil= opt= i= all=
cmd=n sec=\?
cd /usr/man
for i
do
	case $i in

	[1-8])
		sec=$i ;;
	-n)
		cmd=n ;;
	-t)
		cmd=t ;;
	-k)
		cmd=k ;;
	-e | -et | -te)
		cmd=e ;;
	-ek | -ke)
		cmd=ek ;;
	-ne | -en)
		cmd=ne ;;

	-w)
		cmd=where ;;
	-*)
		opt="$opt $i" ;;

	*)
		fil=`echo man$sec/$i.*`
		case $fil in
		man7/eqnchar.7)
			all="$all /usr/pub/eqnchar $fil" ;;

		*\*)
			echo $i not found 1>&2 ;;
		*)
			all="$all $fil" ;;
		esac
	esac
done
case $all in
	"")
		exit ;;
esac
case $cmd in

n)
	tbl $all | nroff $opt -man - | col ;;
ne)
	neqn $all | tbl | nroff $opt -man - | col ;;
t)
	tbl $all | troff $opt -man - ;;
k)
	tbl $all | troff -t $opt -man - | tc ;;
e)
	eqn $all | tbl | troff $opt -man - ;;
ek)
	eqn $all | troff -t $opt -man - | tc ;;

where)
	echo $all ;;
esac

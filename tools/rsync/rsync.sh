
dir_name=$(dirname "$0")
[ "$dir_name" ] || dir_name="."

mkdir_op()
{
	mkdir -p ${1+"$@"} || exit 1
}

cygpath_op()
{
	"$dir_name/bin/cygpath.exe" ${1+"$@"}
}

rsync_op()
{
	mkdir_op "$2"
	src=$(cygpath_op "$1")	
	dest=$(cygpath_op "$2")
	shift
	shift
	"$dir_name/bin/rsync.exe" "$src" "$dest" $@ || { sleep 2; "$dir_name/bin/rsync.exe" "$src" "$dest" $@; } || { sleep 2; "$dir_name/bin/rsync.exe" "$src" "$dest" $@; } || exit 1
}

rsync_op $@


exit 0
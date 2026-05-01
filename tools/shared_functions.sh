# 共享的函数
IS_BUSYBOX=
if [ "$OS" = "Windows_NT" ]; then
	if [ ! "${BASH_SOURCE}" ]; then
		IS_BUSYBOX=true
	fi
elif [ "`uname -s`" = "Darwin" ]; then
	IS_MACOSX=true
fi

# busybox不支持BASH_SOURCE
if [ "$IS_BUSYBOX" = "true" ]; then
	tools_dir=$proj_root_path/tools
else
	tools_dir=$(dirname "${BASH_SOURCE}")
fi

find_command()
{
	which $1 > /dev/null 2>&1
	if [ $? == 0 ]; then
	    echo true
	else
	    echo false
	fi
}

find_function()
{
	if [ "$(type -t $1)" = "function" ]; then
		echo true
	elif [ "$IS_BUSYBOX" = "true" ] && [ "$(type -t $1)" = "$1" ]; then
		echo true
	else
		echo false
	fi
}

iconv_op()
{
	if [ "$(find_command iconv)" = "true" ]; then
		iconv ${1+"$@"}
	else
		"$tools_dir/iconv.exe" ${1+"$@"}
	fi
}

log()
{
	if [ "$IS_BUSYBOX" = "true" ]; then
		echo ${1+"$@"} | iconv_op -f utf-8 -t gbk
	else
		echo ${1+"$@"}
	fi
}
error()
{
	if [ "$IS_BUSYBOX" = "true" ]; then
		log=`echo $@ | iconv_op -f utf-8 -t gbk`
	else
		log="$@"
	fi

	if [ ! $IS_MACOSX ]; then
		echo>&2 -e "\033[;31m$log \033[0m"
	else
		echo>&2 "\033[;31m$log \033[0m"
	fi
}
warn()
{
	if [ "$IS_BUSYBOX" = "true" ]; then
		log=`echo $@ | iconv_op -f utf-8 -t gbk`
	else
		log="$@"
	fi
	if [ ! $IS_MACOSX ]; then
		echo -e "\033[;33m$log \033[0m"
	else
		echo "\033[;33m$log \033[0m"
	fi
}

sessionlog()
{
	if [ "$IS_BUSYBOX" = "true" ]; then
		log=`echo $@ | iconv_op -f utf-8 -t gbk`
	else
		log="$@"
	fi
	if [ ! $IS_MACOSX ]; then
		echo -e "\033[;32m[session] $log \033[0m"
	else
		echo "\033[;32m[session] $log \033[0m"
	fi
}

pause()
{
    #本地运行，暂停提示
    if [ "$LOCAL_RUN" ]; then
		hint="Please Enter Key To Exit."
		if [ ! $IS_MACOSX ]; then
			echo>&2 -n -e "\033[;31m$hint \033[0m\c"
		else
			echo>&2 "\033[;31m$hint \033[0m\c"
		fi
		read
    fi
}

error_pause()
{
	error "$@"
	pause
}

quit_ext()
{
	if [ "$(find_function quit)" = "true" ]; then
		quit ${1+"$@"}
	else
		exit ${1+"$@"}
	fi
}
chmod_op()
{
	chmod ${1+"$@"} || { error_pause "[error] 设置权限失败：chmod $@"; quit_ext 1; }
}

mkdir_op()
{
	mkdir -p ${1+"$@"} || { sleep 2; mkdir -p ${1+"$@"} || { error_pause "[error] 创建目录失败。mkdir $@"; quit_ext 1; } }
}

rm_op()
{
	rm ${1+"$@"} || { sleep 2; rm ${1+"$@"}; } || { sleep 2; rm ${1+"$@"}; } || { error_pause "[error] 删除文件失败：rm $@"; quit_ext 1; }
}

cp_op()
{
	cp ${1+"$@"} || { error_pause "[error] 复制文件失败：cp $@"; quit_ext 1; }
}

mv_op()
{
	mv ${1+"$@"} || { error_pause "[error] 移动文件失败：mv $@"; quit_ext 1; }
}

busybox_op()
{
	"$tools_dir/busybox.exe" ${1+"$@"}
}

svn_op()
{
	log "svn $@"
	[ "$tmp" ] || tmp="."
	
	retry=0
	while [ "$retry" -lt "20" ]; do
		svn 2>$tmp/svn-stderr --non-interactive ${1+"$@"}
		
		if [ -s $tmp/svn-stderr ]; then
			svn_err=`cat $tmp/svn-stderr`
			
			if [ "$svn_err" != "${svn_err/SASL(-13)/}" ]; then   #svn_err contains "SASL(-13)"
				echo "svn 操作失败，即将重试: svn $@: $svn_err"
				sleep 5
				retry=$(($retry+1))
			else
				error "[error] svn 操作失败: svn $@: "
				error_pause "$svn_err"
				quit_ext 1
			fi
		else
			break
		fi
	done
}

xcopy_op()
{
	xcopy ${1+"$@"} | iconv_op -f gbk -t utf-8 ||  { error_pause "[error] xcopy 复制文件失败：xcopy $@"; quit_ext 1; }
}

netcat_op()
{
	log "nc $@"
	if [ "$(find_command nc)" = "true" ]; then
		nc ${1+"$@"} || { error "[error] netcat 操作失败 netcat $@"; }
	else
		busybox_op nc ${1+"$@"} || { error "[error] netcat 操作失败 busybox_op nc $@"; }
	fi
}

unity_op()
{
	log "unity $@"
	
	# 选择一个端口
	port=$(($$ % 65535))
	[ $port -lt 1000 ] && port=$((port+9000))
	[ $port -lt 10000 ] && port=$((port+50000))
	
	log "consoleoutput port: $port"
	
	rm -f "$tmp/unity_ret"
	rm -f "$tmp/console_output_flag"
	
	# 管道前部分：调用 Unity，指定 consoleoutput 地址；把 Unity 返回码写入文件；等一下监听建立；连一下 consoleoutput 监听地址，以免一直等待。管道后部分：监听 consoleoutput 连接，并输出 log
	if [ "$OS" = "Windows_NT" ]; then
			{ "$unity" --consoleoutput "127.0.0.1:$port" -logFile "$tmp/unity.log" ${1+"$@"}; echo "$?">"$tmp/unity_ret"; sleep 1; echo "front">"$tmp/console_output_flag"; warn "[warning] Unity did not connect console output" | netcat_op 127.0.0.1 $port 2>"$tmp/null"; } \
		| { netcat_op -l -p $port 127.0.0.1; echo "back">"$tmp/console_output_flag"; }
	else
			{ "$unity" --consoleoutput "127.0.0.1:$port" -logFile "$tmp/unity.log" ${1+"$@"}; echo "$?">"$tmp/unity_ret"; sleep 1; echo "front">"$tmp/console_output_flag"; warn "[warning] Unity did not connect console output" | netcat_op 127.0.0.1 $port 2>"$tmp/null"; } \
		| { netcat_op -l $port; echo "back">"$tmp/console_output_flag"; }
	fi
	ret=$(cat "$tmp/unity_ret")
	
	console_output_flag=$(cat "$tmp/console_output_flag")
	if [ "$console_output_flag" = "back" ]; then	#管道后部进程后结束，表明无 console output，输出文件 log
		cat "$tmp/unity.log"
	fi
	
	if [ ! "$ret" = "0" ]; then
		error_pause "[error] unity 调用失败：unity $@"
		quit_ext 1
	fi
}

zip_op()
{
	if [ "$(find_command zip)" = "true" ]; then
		zip ${1+"$@"} || { error_pause "[error] zip 失败：zip $@"; quit_ext 1; }
	else
		"$tools_dir/zip/zip.exe" ${1+"$@"} || { error_pause "[error] zip.exe 失败：zip $@"; quit_ext 1; }
	fi
}
cygpath_op()
{
	"$tools_dir/rsync/bin/cygpath.exe" ${1+"$@"}
}

# sync_op <src> <dest> [options ...]。效果同 rsync
sync_op()
{
	if [ "$OS" = "Windows_NT" ]; then
		if [ "$IS_BUSYBOX" = "true" ]; then
			mkdir_op "$2"
			src=$(cygpath_op "$1")	
			dest=$(cygpath_op "$2")
			shift
			shift
			rsync_bin="$tools_dir/rsync/bin/rsync.exe"
			"$rsync_bin" "$src" "$dest" ${1+"$@"} || { sleep 2; "$rsync_bin" "$src" "$dest" ${1+"$@"}; } || { sleep 2; "$rsync_bin" "$src" "$dest" ${1+"$@"}; } || { error_pause "[error] rsync 同步文件失败：rsync.exe "$src" "$dest" $@"; quit_ext 1; }
		else
			busybox_op sh "$tools_dir/rsync/rsync.sh" ${1+"$@"} || { error_pause "[error] rsync 失败 $@"; quit_ext 1; }
		fi
	else
		src=$1
		dest=$2
		shift
		shift
		rsync "$src" "$dest" $@ || { sleep 2; rsync "$src" "$dest" $@; } || { sleep 2; rsync "$src" "$dest" $@; } || { error_pause "[error] rsync 同步文件失败：rsync "$src" "$dest" $@"; quit_ext 1; }
	fi
}

curl_op()
{
	if [ ! "$(find_command curl)" = "true" ]; then
		error_pause "[error] curl 工具未找到"
		quit_ext 1
	fi
	curl -i -s -S ${1+"$@"} | iconv -f gbk -t utf-8
	if [ ! $? ]; then
		error_pause "[error] curl 操作失败: curl $@"
		quit_ext 1
	fi
}
# 应用差分
apply_diff_op()
{
	project_path=${1%/}
	diff_name=$2
	sub_path=$3

	oldIFS=$IFS
	IFS=","
	for diff_part in $diff_name
	do
		[ "$oldIFS" ] && { IFS=$oldIFS; oldIFS=""; }
		
		diff_dir="$project_path/diffconfigs/$diff_part"
		
		[ -d "$diff_dir" ] || { error_pause "[error] 差分目录不存在 $diff_dir"; quit_ext 1; }
		
		if [ ! "$sub_path" ]; then
			[ "$(ls -A $diff_dir)" ] && cp_op -rf "$diff_dir/"* "$project_path/"
		else
			[ "$(ls -A $diff_dir/$sub_path)" ] && cp_op -rf "$diff_dir/$sub_path/"* "$project_path/$sub_path/"
		fi
	done
}

#所有文件名和目录都转成小写
tolower_op()
{
	make_tolower()
	{
		str=`echo $1|tr 'A-Z' 'a-z'`
		mv -f $1 $str || { error_pause "[error] tolower 修改文件名$1为小写。"; quit_ext 1; }
	}
	dir_tolower()
	{
		for file in `ls $1 | grep '[A-Z]'`
		do
			str=`echo $file|tr 'A-Z' 'a-z'`
			mv -f $1/$file $1/$str || { error_pause "[error] tolower 修改文件名为小写。"; quit_ext 1; }
		done
	}

	if [ -f $1 ]; then
		make_tolower $1
	else
		dir_tolower $1
		for file in $1/* 
		do
			if [ -d $file ]; then
				tolower_op $file
			fi
		done
		make_tolower $1
	fi
}

lower_op()
{
	log "tolower $@"
	tolower_op ${1+"$@"} || { error_pause "[error] lower 修改文件名为小写 tolower $@"; quit_ext 1; }
}

isworkcopy_op()
{
	svn info $1 > /dev/null 2>&1
	if [ $? == 0 ]; then
	    echo true
	else
	    echo false
	fi
}

svnremovemissing()
{
	FILES=
	removemissing()
	{
		STATUS_URL="svn_op status --ignore-externals $1"
		STATUS_NUM=`${STATUS_URL} |wc -l`
		if [ ${STATUS_NUM} -ne 0 ]; then
			STATUS_LIST=`${STATUS_URL}`
			NUM=0
			HANDLE=0
			FLAG=
			for FIELD in ${STATUS_LIST} ; do
				if [ ${#FIELD} -lt 3 ]; then
					FLAG=$FIELD
					#!是missing,?是本地文件，X是外链目录,D是删除状态
					if [ "${FIELD}" == "!" ]; then
						#下一个应该处理
						let NUM+=1
						HANDLE=1
					# elif [ "${FIELD}" == "?" ]; then
					# 	let NUM+=1
					# 	HANDLE=1
					elif [ "${FIELD}" == "X" ]; then
						let NUM+=1
						HANDLE=1
					# elif [ "${FIELD}" == "D" ]; then
					# 	let NUM+=1
					# 	HANDLE=1
					fi
					continue
				fi

				#若为删除文件则不必导出
				if [ ! ${HANDLE} -eq 1 ]; then
					continue
				fi
				if [ "$OS" = "Windows_NT" ]; then
					FILE=$(echo "$FIELD" | sed -r 's/\//\\/g')
				else
					FILE=$FIELD
				fi
				HANDLE=0
				if [ "$FLAG" = "X" ]; then
					removemissing ${FILE}
				elif [ "$FLAG" = "?" ]; then
					rm_op -rf ${FILE}
				elif [ "$FLAG" = "!" ]; then
					svn_op delete -q --force ${FILE}
					FILES=${FILES}" ${FILE}"
				elif [ "$FLAG" = "D" ]; then
					FILES=${FILES}" ${FILE}"
				fi
			done
		fi
	}

	if [ ! "$(isworkcopy_op $1)" = "true" ]; then
		error_pause "path is not svn working folder: $1"
		quit_ext 1
	fi
	removemissing $1
	#[ "$FILES" ] && svn_op commit -m "auto removemissing" $FILES
}


svnremovemissing_op()
{
	#可以统一全用shell
	svnremovemissing ${1+"$@"} || { error_pause "[error] svnremovemissing $@"; quit_ext 1; }
}

#自动解决冲突的更新
svnup_auto_fix_confict_op()
{
	if [ ! "$(isworkcopy_op $1)" = "true" ]; then
		error_pause "path is not svn working folder: $1"
		quit_ext 1
	fi
	svn_op cleanup $1 --include-externals
	svn_op update $1
	# match conflict | locally deleted | property conflict
	svn_op status -q $1|grep -E "^C. |^D. |^.C " >$tmp/confict.txt
	for FIELD in `cat "$tmp/confict.txt"`
	do
	   if [ ${#FIELD} -lt 3 ]; then
	       continue
	   fi
	   svn_op revert "$FIELD"
	done
}


xcodebuild_op()
{
	log xcodebuild {1+"$@"}
	xcodebuild ${1+"$@"} || { error_pause "[error] xcodebuild 操作失败：xcodebuild $@"; quit_ext 1; }
}

# 计算文件md5
calcfile_md5_op()
{
	if [ "$(find_command md5sum)" = "true" ]; then
		echo `md5sum $1 | awk -F " " '{print $1}'`
	elif [ "$(find_command md5)" = "true" ]; then
		echo `md5 $1 | awk -F " = " '{print $2}'`
	else
		echo ""
	fi
}
#oldmd5code=$(calcfile_md5_op "$out_dir/PB/client_msg.lua")

abspath_op()
{
	# 给定某个路径，输出绝对路径
	# $1 给定路径
	src_path=$1

	if [ ! "$src_path" ]; then
		base_name=`basename "$0"`
		echo "usage: $base_name <路径>"
	elif [ "${src_path#/}" = "$src_path" ] && [ "${src_path#\\}" = "$src_path" ] && [ "${src_path#*:}" = "$src_path" ]; then	#不以 '/', '\' 开头，且不含 ':'，是相对路径
		echo "$(pwd)/$src_path"
	else
		echo "$src_path"
	fi
}

# RunUAT.bat, 需要指定UE_ROOT
UAT_op()
{
	log "RunUAT $@"
	
	if [ ! $UE_ROOT ]; then
		error_pause "UE_ROOT未定义"
		quit_ext 1
	fi
	if [ ! $IS_MACOSX ]; then
		"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.bat" ${1+"$@"} || { error_pause "[error] RunUAT.bat失败。RunUAT.bat $@"; quit_ext 1; }
	else
		sh "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ${1+"$@"} || { error_pause "[error] RunUAT.sh失败。RunUAT.sh $@"; quit_ext 1; }
	fi
}
# UnrealBuildTool.exe, 需要指定UE_ROOT
UBT_op()
{
	log "UnrealBuildTool $@"
	if [ ! $UE_ROOT ]; then
		error_pause "UE_ROOT未定义"
		quit_ext 1
	fi

	if [ ! $IS_MACOSX ]; then
		"$UE_ROOT/Engine/Binaries/DotNET/UnrealBuildTool.exe" ${1+"$@"} || { error_pause "[error] UnrealBuildTool.exe失败。UnrealBuildTool.exe $@"; quit_ext 1; }
	else
		sh "$UE_ROOT/Engine/Build/BatchFiles/Mac/RunMono.sh" "$UE_ROOT/Engine/Binaries/DotNET/UnrealBuildTool.exe" ${1+"$@"} || { error_pause "[error] UnrealBuildTool.exe失败。UnrealBuildTool.exe $@"; quit_ext 1; }
	fi
}
# UE4Editor-Cmd.exe, 需要指定UE_ROOT
UE_op()
{
	log "UE4Editor-Cmd $@"
	if [ ! $UE_ROOT ]; then
		error_pause "UE_ROOT未定义"
		quit_ext 1
	fi

	if [ ! $IS_MACOSX ]; then
		"$UE_ROOT/Engine/Binaries/Win64/UE4Editor-Cmd.exe" ${1+"$@"} || { error_pause "[error] UE4Editor-Cmd.exe失败。UE4Editor-Cmd.exe $@"; quit_ext 1; }
	else
		"$UE_ROOT/Engine/Binaries/Mac/UE4Editor.app" ${1+"$@"} || { error_pause "[error] UE4Editor-Cmd.exe失败。UE4Editor.app $@"; quit_ext 1; }
	fi
}

connect_wifi()
{
	return  #这样其实连不上，而且可能断连
	
	profile_name=$1
	if [ ! "$profile_name" ]; then
		profile_name=Archosaur
	fi
	#有 wifi 接口时才尝试连接，不过没有 wifi 设备时，所有 netsh wlan 命令似乎都会成功
	{ netsh wlan show profile name=$profile_name >/dev/null; } || return 0
	iTry=0
	while [ $iTry -lt 20 ]; do
		{ netsh wlan connect name=$profile_name >/dev/null; } && return 0
		iTry=$(($iTry + 1))
		sleep 1
		echo "重试连接 wifi '$profile_name' #$iTry"
	done
	error "连接 wifi '$profile_name' 失败"
	quit_ext 1
}

#xmake 编译
xmake_op()
{
	if [ ! "$XMAKE_HOME" ]; then
		source "$tools_dir/get_xmake.sh"
	fi
	if [ ! "$XMAKE_HOME" ]; then
		error "XMAKE_HOME Not Define."
		quit_ext 1
	fi

	log "xmake $@" 
	
	if [ "$OS" = "Windows_NT" ]; then
		"$XMAKE_HOME/xmake.exe" ${1+"$@"} || { error "error build"; quit_ext 1; }
	else
		"$XMAKE_HOME/xmake" ${1+"$@"} || { error "error build"; quit_ext 1; }
	fi
}

msbuild_op()
{
	log "msbuild $@"
	if [ "$LOCAL_RUN" ]; then
		"$msbuild" ${1+"$@"}
	else
		"$msbuild" ${1+"$@"} #| iconv_op -f gbk -t utf-8
	fi
	ret_code=$?
  	if [ $ret_code -ne 0 ]; then
  		error_pause "[error] msbuild失败。msbuild $@"
  		quit_ext 1
  	fi
}

get_msbuild_op()
{
	# if [ "$1" = "csharp" ]; then
	# 	if [ -f "$WINDIR/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe" ]; then
	# 		MS_BUILD="$WINDIR/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
	# 	fi
	# else
		if [ -d "$VS141COMNTOOLS" ]; then
			COMNTOOLS=${VS141COMNTOOLS%/}
			COMNTOOLS=${VS141COMNTOOLS%\\}
			MS_BUILD="${COMNTOOLS}/../../MSBuild/15.0/Bin/MSBuild.exe"
		else
			MS_BUILD=`$tools_dir/GetMSBuildPath.bat bash`
		fi
	#fi
}

buildconsole_op()
{
	log "BuildConsole: $@"
	if [ "$LOCAL_RUN" ]; then
		BuildConsole ${1+"$@"}
	else
		BuildConsole ${1+"$@"} #| iconv_op -f utf-8 -t gbk
	fi
	ret_code=$?
  	if [ $ret_code -ne 0 ]; then
  		error_pause "[error] BuildConsole. BuildConsole $@"
  		quit_ext 1
  	fi
}

lua_op()
{
	if [ "$(find_command lua)" = "true" ]; then
		lua ${1+"$@"} || { error_pause "[error] lua 失败：lua $@"; quit_ext 1; }
	else
		error_pause "[error] lua not found."; quit_ext 1;
	fi
}

has_space()
{
	# if [ "$1" =~ \ |\' ]; then #有空格
	# 	echo true
	# else
	# 	echo false
	# fi
	case "$1" in 
	   *\ * ) echo true ;;
	   *) echo false ;;
	esac
}
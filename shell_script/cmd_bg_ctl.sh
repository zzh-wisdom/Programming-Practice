#!/bin/bash

cluster=""
qps=1000
pid_file=""

get_param()
{
    ind=0
    OPTS=""
    for param in $@
    do
        if [ $ind -gt 0 ];
        then
            OPTS="$OPTS $param"
        fi
        ((ind++))
     done
    echo $OPTS
}

run_cmd_bg() 
{
    # echo "全部参数:" $*
    # echo "参数0:" $0  # 脚本文件名本身
    echo "run cmd:" $*
    # echo "pid_file:" $2
    # echo "cluster:" ${cluster}
    # echo "qps:" ${qps}

    nohup $* > /dev/null 2>&1 &

    echo $! > ${pid_file}

    pid=`cat ${pid_file}`
    echo "start success, pid_file: ${pid_file}, [$pid]"
}

# 参数1: 服务名
# 参数2: qps
start()
{
    ulimit -c unlimited

    pushd . > /dev/null   # 简单理解为当前的工作目录（以堆栈的形式管理，popd后回到上次工作目录）

    cd `dirname $0`
    # mkdir -p logs

    # sed -i "5c \ \ \ \ \ \ \ \ \ \ \ \ \ \ value=\"`pwd`/logs\"" conf/logback.xml
    # sed -i '30c \ \ \ \ <root level="WARN">' conf/logback.xml
    # sed -i '31c \ \ \ \ \ \ \ \ <appender-ref ref="FILE" />' conf/logback.xml
    
    cluster=${1}
    qps=${2}
    cmd="./redis_go_sdk_bench -cluster ${cluster} -cmd set -key_count 100000 -client_count 100 -qps ${qps}"
    pid_file="set.pid"
    run_cmd_bg ${cmd}
    popd
}

stop()
{
    pushd . > /dev/null

    cd `dirname $0`
    pid=`cat set.pid`
    pkill -F set.pid
    unlink set.pid
    echo "stop process, pid: $pid"
    popd
}

case C"$1" in
    C)
        echo "Usage: $0 {start|stop} <服务名> <qps>"
        ;;
    Cstart)
        var_param=$(get_param $*)
        start $var_param
        echo "Done!"
        ;;
    Cstop)
        stop
        echo "Done!"
        ;;
    C*)
        echo "Usage: $0 {start|stop}"
        ;;
esac

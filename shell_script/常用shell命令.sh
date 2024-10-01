# ps 查询某个进程的pid，并kill。注意label需要全局唯一
label=${1}
ps aux | grep ${label} | grep -v grep | awk '{print $2}' | xargs kill -9
ps aux | grep ${label}
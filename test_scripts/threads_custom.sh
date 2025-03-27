#!/usr/bin/env bash

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

if [[ `check_dir` -eq 1 ]]; then
    exit 1
fi


port=`get_port`

if [[ port -eq 1 ]]; then
    exit 1
fi

# Start up server.
./httpserver -t 2 $port > output.txt 2>error.txt &
pid=$!

# Wait until we can connect.
wait_for_listen $port
wait_rc=$?

if [[ $wait_rc -eq 1 ]]; then
    echo "Server didn't listen on $port in time"
    kill -9 $pid
    wait $pid &>/dev/null
    exit 1
fi

rc=0
count=`ps -o thcount $pid | head -2 | tail -1 | cut -d ' ' -f 5`
if [ $count -ne 3 ]; then
    msg="Server created $count threads instead of 2 threads\n"
    rc=1
fi

# Clean up.
## Make sure the server is dead.
kill -9 $pid
wait $pid &>/dev/null

if [[ $rc -ne 0 ]]; then
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat error.txt
    echo "--------------------------------------------------------------------------------"
fi


port=`get_port`

if [[ port -eq 1 ]]; then
    exit 1
fi

# Start up server.
./httpserver $port > output.txt 2>error.txt &
pid=$!

# Wait until we can connect.
wait_for_listen $port
wait_rc=$?

if [[ $wait_rc -eq 1 ]]; then
    echo "Server didn't listen on $port in time"
    kill -9 $pid
    wait $pid &>/dev/null
    exit 1
fi

rc=0
count=`ps -o thcount $pid | head -2 | tail -1 | cut -d ' ' -f 5`
if [ $count -ne 5 ]; then
    msg="${msg}Server created $count threads instead of 4 threads\n"
    rc=1
fi

# Clean up.
## Make sure the server is dead.
kill -9 $pid
wait $pid &>/dev/null

if [[ $rc -ne 0 ]]; then
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat error.txt
    echo "--------------------------------------------------------------------------------"
fi


port=`get_port`

if [[ port -eq 1 ]]; then
    exit 1
fi

# Start up server.
./httpserver -t 8 $port > output.txt 2>error.txt &
pid=$!

# Wait until we can connect.
wait_for_listen $port
wait_rc=$?

if [[ $wait_rc -eq 1 ]]; then
    echo "Server didn't listen on $port in time"
    kill -9 $pid
    wait $pid &>/dev/null
    exit 1
fi

rc=0
count=`ps -o thcount $pid | head -2 | tail -1 | cut -d ' ' -f 5`
if [ $count -ne 9 ]; then
    msg="${msg}Server created $count threads instead of 8 threads\n"
    rc=1
fi

# Clean up.
## Make sure the server is dead.
kill -9 $pid
wait $pid &>/dev/null

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "$msg"
    echo "--------------------------------------------------------------------------------"
    echo "stdout:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "stderr:"
    cat error.txt
    echo "--------------------------------------------------------------------------------"
fi

exit $rc

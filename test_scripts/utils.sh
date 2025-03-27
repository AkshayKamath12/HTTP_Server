cleanup() {
    for file in $@; do
	rm -r $file
    done
}

check_dir() {
    if [ ! -f httpserver ]; then
	echo "I cannot find httpserver binary.  Did you..."
	echo "(1) build your assignment?"
	echo "(2) run this script from your assignment directory?"
	return 1
    fi
    return 0
}



get_port() {
    #### usable port range 32768 -> 65535

    ss --tcp state CLOSE-WAIT --kill &>/dev/null
    ss --tcp state TIME-WAIT --kill &>/dev/null
    getports=`netstat -antu | tail -n +3 | awk '{split($4, parts,":"); print parts[length(parts)]}' | uniq`

    declare -A portmap
    for port in $getports; do
	portmap[$port]=1
    done

    return_port=32768
    while [[ $return_port -le 65535 ]]; do
	if [[ ! -v portmap[$return_port] ]]; then
            echo $return_port
            exit 0
	fi
	((return_port+=1))
    done

    echo "couldn't find port"
    exit 1
}


wait_for_listen() {
    port=$1
    count=1

    while [[ `ss -tlH sport = :$port | wc -l` -lt 1 && "$count" -lt 100 ]]; do
	sleep .05
	count=$(($count + 1))
    done

    if [[ "$count" -eq 40 ]]; then
	return 1
    fi

    return 0
}

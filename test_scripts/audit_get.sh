#!/usr/bin/env bash

source_dir=`dirname ${BASH_SOURCE}`

workload=$source_dir/../workloads/audit_get.toml
nthreads=1

$source_dir/test_workload.sh $workload $nthreads
exit $?

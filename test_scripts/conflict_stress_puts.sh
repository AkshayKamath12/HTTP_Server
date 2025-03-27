#!/usr/bin/env bash

source_dir=`dirname ${BASH_SOURCE}`

workload=$source_dir/../workloads/conflict_stress_put.toml
nthreads=4

$source_dir/test_workload.sh $workload $nthreads
exit $?

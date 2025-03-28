#!/bin/bash

name="binder_test"

args_compile=()

add_range_to_args_compile() {
    local start=$1
    local end=$2
    for ((i=start; i<=end; i++)); do
        args_compile+=("$i")
    done
}

add_range_to_args_compile 101 110
add_range_to_args_compile 201 202
add_range_to_args_compile 301 306
add_range_to_args_compile 401 407
add_range_to_args_compile 501 513
add_range_to_args_compile 601 604

#args_nocompile=()

echo 'SHOULD COMPILE:'

for arg in "${args_compile[@]}"; do
    ./test_single.sh $name $arg
done

#echo 'SHOULDNT COMPILE:'

#for arg in "${args_nocompile[@]}"; do
#    echo 'Compiling $name -DTEST_NUM='$arg'...'
#    ./compile_test.sh $name "$arg" 2>/dev/null
#    if [ $? -eq 0 ]; then
#        echo "Compilation succeeded for TEST_NUM=$arg, but it should have failed."
#    else
#        echo "Compilation failed for TEST_NUM=$arg as expected."
#    fi
#    echo
#done
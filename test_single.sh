#!/bin/bash

name=$1
arg=$2

echo "Compiling $name -DTEST_NUM=$arg..."
./compile_test.sh $name "$arg"

if [ $? -ne 0 ]; then
    echo -e "\e[31mCompilation failed\e[0m"
else
    echo -e "\e[32mCompilation finished successfully\e[0m"
fi

echo 'Running...'
./a.out

if [ $? -ne 0 ]; then
    echo -e "\e[31mRunning failed\e[0m"
else
    echo -e "\e[32mRunning finished successfully\e[0m"
fi

echo 'Running finished. Running with valgrind...'
valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --run-cxx-freeres=yes -q ./a.out 2>/dev/null

if [ $? -ne 0 ]; then
    echo -e "\e[31mValgrind failed\e[0m"
else
    echo -e "\e[32mValgrind finished successfully\e[0m"
fi

echo 'Done.'
echo
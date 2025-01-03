#!/bin/bash

g++ -Wall -Wextra -O2 -std=c++20 $1.cpp -o a.out -DTEST_NUM=$2
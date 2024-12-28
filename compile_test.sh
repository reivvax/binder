#!/bin/bash

g++ -Wall -Wextra -O2 -std=c++20 binder_example.cpp -o binder_example
g++ -Wall -Wextra -O2 -std=c++20 binder_simple_tests.cpp -o binder_simple_tests
g++ -Wall -Wextra -O2 -std=c++20 binder_iterator_tests.cpp -o binder_iterator_tests
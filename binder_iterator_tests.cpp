#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "binder.h"

// Pomocnicza funkcja testująca - raportuje wynik
void assert_true(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "PASS: " << test_name << std::endl;
    } else {
        std::cout << "FAIL: " << test_name << std::endl;
    }
}

// gpt_tests
void iterator_tests() {

    // Test 1: iterator conent
    {
        cxx::binder<std::string, int> binder;
        binder.insert_front("key1", 10);
        binder.insert_front("key2", 20);
        binder.insert_front("key3", 30);

        std::vector<std::pair<std::string, int>> expected = {
            {"key3", 30},
            {"key2", 20},
            {"key1", 10}
        };

        auto it = binder.cbegin();
        for (const auto& [key, value] : expected) {
            assert_true(it != binder.cend(), "Iterator should not reach the end prematurely");
            assert_true(*it == value, "Iterator content mismatch");
            ++it;
        }
        assert_true(it == binder.cend(), "Iterator should reach the end after traversal");
    }

    // Test 2: forward iterator
    {
        cxx::binder<std::string, int> binder;
        binder.insert_front("key1", 10);
        binder.insert_front("key2", 20);

        static_assert(std::forward_iterator<decltype(binder.cbegin())>, "Iterator must meet forward_iterator requirements");

        auto it1 = binder.cbegin();
        auto it2 = it1;

        assert_true(it1 == it2, "Copied iterators should be equal");
        ++it1;
        assert_true(it1 != it2, "Modified iterator should not be equal to the original");
    }

    // Test 3: empty binder iteration
    {
        cxx::binder<std::string, int> binder;

        auto it = binder.cbegin();
        assert_true(it == binder.cend(), "Iterator should be equal to end on an empty binder");
    }

}


int main() {
    iterator_tests();
    return 0;
}
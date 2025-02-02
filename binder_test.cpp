// Kompilowanie i uruchamianie
// g++ -Wall -Wextra -O2 -std=c++20 -DTEST_NUM=... binder_test*.cpp -o binder_test && ./binder_test && valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --run-cxx-freeres=yes -q ./binder_test

#include "binder.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string>
#include <string_view>

using cxx::binder;

using std::as_const;
using std::align_val_t;
using std::bad_alloc;
using std::clog;
using std::forward_iterator;
using std::invalid_argument;
using std::make_unique;
using std::move;
using std::pair;
using std::make_pair;
using std::swap;
using std::string;
using std::string_view;
using std::vector;

int main();

namespace {
#if TEST_NUM == 102 || TEST_NUM == 105 || TEST_NUM == 402 || TEST_NUM == 403 || \
    (TEST_NUM > 500 && TEST_NUM <= 599) || TEST_NUM == 603
  // Sprawdzamy równość (notatek) skoroszytów.
  // To wymaga poprawnego działania części metod oraz iteratora.
  // Sprawdzenie zakładek jest utrudnione przez brak bezpośredniego dostępu
  template <typename K, typename V>
  bool operator==(binder<K, V> const &b1, binder<K, V> const &b2) {
    size_t b1_size = b1.size();
    size_t b2_size = b2.size();

    if (b1_size != b2_size)
      return false;

    binder<K, V> local1 = b1;
    binder<K, V> local2 = b2;

    typename binder<K, V>::const_iterator it1 = b1.cbegin(), end1 = b1.cend(),
                                         it2 = b2.cbegin(), end2 = b2.cend();
    for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
      if (*it1 != *it2)
        return false;
    }
    if (it1 != end1 || it2 != end2)
      return false;

    return true;
  }

  // Porównanie dwóch skoroszytów włącznie z kluczami (dołączonymi jako argument funkcji)
  template <typename K, typename V>
  bool check_with_keys(binder<K, V> const &b1, binder<K, V> const &b2, vector<K> const &keys) {
    size_t b1_size = b1.size();
    size_t b2_size = b2.size();
    size_t keys_count = keys.size();

    if (b1_size != b2_size)
      return false;

    if (b1_size != keys_count)
      return false;

    // Sprawdzamy równość za pomocą iteratorów
    typename binder<K, V>::const_iterator it1 = b1.cbegin(), end1 = b1.cend(),
                                          it2 = b2.cbegin(), end2 = b2.cend();
    for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
      // W testach używamy klasy V, której obiekty można porównywać.
      if (*it1 != *it2)
        return false;
    }
    if (it1 != end1 || it2 != end2)
      return false;

    // Sprawdzamy za pomocą zakładek.
    typename vector<K>::const_iterator kit = keys.cbegin(), kend = keys.cend();
    try {
      for (; kit != kend; ++kit) {
        // W testach używamy klasy V, której obiekty można porównywać.
        if (b1.read(*kit) != b2.read(*kit))
          return false;
      }
    }
    catch (...) {
      return false;
    }

    // Jeszcze raz kopiujemy i sprawdzamy za pomocą read oraz remove().
    // Jeśli zakładki pojawiają w innej kolejności niż w keys podniesiony zostanie wyjątek.
    binder<K, V> local1 = b1;
    binder<K, V> local2 = b2;
    kit = keys.cbegin();
    try {
      for (; kit != kend; ++kit) {
        // W testach używamy klasy V, której obiekty można porównywać.
        if (local1.read(*kit) != local2.read(*kit))
          return false;
        local1.remove();
        local2.remove();
      }
    }
    catch (...) {
      return false;
    }
    return true;
  }
#endif

#if TEST_NUM == 102
  auto foo(binder<int, int> q) {
    return q;
  }
#endif

#if TEST_NUM == 103 || TEST_NUM == 107 || TEST_NUM == 701
  // Potwierdzenie, że notatka o podanej zakładce nie znajduje się w skoroszycie.
  template <typename K, typename V>
  void assert_not_in(binder<K, V> const &bi, K const &k) {
    bool catched = false;
    try {
      as_const(bi).read(k);
    }
    catch(invalid_argument const &) {
      catched = true;
    }
    assert(catched);
  }
#endif

#if TEST_NUM == 103
  template <typename K, typename V>
  void test_insert_front_spec(binder<K, V> &bi, K const &k, V v) {
    binder<K, V> bi_old = bi;

    assert_not_in(bi_old, k);
    bi.insert_front(k, v);
    assert(bi.size() == bi_old.size() + 1);
    assert(as_const(bi).read(k) == v);
  }

  template <typename K, typename V>
  void test_insert_after_spec(binder<K, V> &bi, K const &k1, K const &k2, V v) {
    binder<K, V> bi_old = bi;

    assert_not_in(bi_old, k2);
     bi.insert_after(k1, k2, v);
    assert(bi.size() == bi_old.size() + 1);
    assert(as_const(bi).read(k2) == v);
  }

  template <typename K, typename V>
  void check_remove_key(binder<K, V> &bi_old, binder<K, V> &bi, K const &k) {
    int diff_count = 0;

    assert(bi.size() + 1 == bi_old.size());

    V const & v = as_const(bi_old).read(k);

    assert_not_in(bi, k);

    typename binder<K, V>::const_iterator it1 = bi_old.cbegin(), end1 = bi_old.cend(),
                                          it2 = bi.cbegin(), end2 = bi.cend();
    for (; it1 != end1; ++it1) {
      if ((it2 == end2) || (*it1 != *it2)) {
        assert(*it1 == v);
        diff_count++;
      } else {
        ++it2;
      }
    }
    assert(it2 == end2);
    assert(diff_count == 1);
  }

  template <typename K, typename V>
  void test_remove_key_spec(binder<K, V> &bi, K const &k) {
    binder<K, V> bi_old = bi;

    bi.remove(k);

    check_remove_key(bi_old, bi, k);
  }

  // Klucz podany, aby sprawdzić, czy poprawny element został usunięty.
  template <typename K, typename V>
  void test_remove_spec(binder<K, V> &bi, K const &k) {
    binder<K, V> bi_old = bi;

    bi.remove();

    check_remove_key(bi_old, bi, k);
  }
#endif

#if TEST_NUM == 104 || TEST_NUM == 106 || TEST_NUM == 201 || TEST_NUM == 202 || TEST_NUM > 400
  int throw_countdown;
  bool throw_checking = false;

  void ThisCanThrow() {
    if (throw_checking && --throw_countdown <= 0)
      throw 0;
  }
#endif

#if TEST_NUM == 104 || TEST_NUM == 201 || TEST_NUM == 202 || TEST_NUM > 400
  class Key {
    friend int ::main();

  public:
    explicit Key(size_t key = 0) : key(key) {
      ThisCanThrow();
      ++instance_count;
      ++operation_count;
    }

    Key(Key const &other) {
      ThisCanThrow();
      key = other.key;
      ++instance_count;
      ++operation_count;
    }

    Key(Key &&other) {
      key = other.key;
      ++operation_count;
    }

    Key & operator=(Key other) {
      ThisCanThrow();
      key = other.key;
      ++operation_count;
      return *this;
    }

    ~Key() {
      --instance_count;
      ++operation_count;
    }

    auto operator<=>(Key const &other) const {
      ThisCanThrow();
      ++operation_count;
      return key <=> other.key;
    }

    bool operator==(Key const &other) const {
      ThisCanThrow();
      ++operation_count;
      return key == other.key;
    }

  private:
    inline static size_t instance_count = 0;
    inline static size_t operation_count = 0;
    size_t key;
  };
#endif

#if TEST_NUM == 104 || TEST_NUM == 106 || TEST_NUM == 201 || TEST_NUM == 202 || TEST_NUM > 400
  class Value {
    friend int ::main();

  public:
    Value(Value const &other) {
      ThisCanThrow();
      value = other.value;
      ++instance_count;
      ++operation_count;
    }

    ~Value() {
      --instance_count;
      ++operation_count;
    }

  private:
    inline static size_t instance_count = 0;
    inline static size_t operation_count = 0;
    size_t value;

    explicit Value(size_t value) : value(value) {
      ++instance_count;
      ++operation_count;
    }

    Value(Value &&other) = delete;
    Value & operator=(Value const &other) = delete;
    Value & operator=(Value &&other) = delete;

#if TEST_NUM > 400 && TEST_NUM <= 599
  public:
    auto operator<=>(Value const &other) const {
      ++operation_count;
      return value <=> other.value;
    }

    bool operator==(Value const &other) const {
      ++operation_count;
      return value == other.value;
    }
#endif
  };
#endif

#if TEST_NUM > 400 && TEST_NUM <= 499
  template <typename binder, typename Operation>
  void NoThrowCheck(binder &b, Operation const &op, char const *name) {
    try {
      throw_countdown = 0;
      throw_checking = true;
      op(b);
      throw_checking = false;
    }
    catch (...) {
      throw_checking = false;
      clog << "Operacja " << name << " podnosi wyjątek.\n";
      assert(false);
    }
  }
#endif

#if TEST_NUM > 500 && TEST_NUM <= 599
  template <typename binder, typename Operation>
  bool StrongCheck(binder &b, binder const &d, Operation const &op, char const *name) {
    bool succeeded = false;

    try {
      throw_checking = true;
      op(b);
      throw_checking = false;
      succeeded = true;
    }
    catch (...) {
      throw_checking = false;
      bool unchanged = d == b;
      if (!unchanged) {
        clog << "Operacja " << name << " nie daje silnej gwarancji\n.";
        assert(unchanged);
      }
    }

    return succeeded;
  }
#endif
} // koniec anonimowej przestrzeni nazw

// Operatory new nie mogą być deklarowane w anonimowej przestrzeni nazw.
#if TEST_NUM > 400 && TEST_NUM <= 599
void* operator new(size_t size) {
  try {
    ThisCanThrow();
    void* p = malloc(size);
    if (!p)
      throw "malloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new(size_t size, align_val_t al) {
  try {
    ThisCanThrow();
    void* p = aligned_alloc(static_cast<size_t>(al), size);
    if (!p)
      throw "aligned_alloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new[](size_t size) {
  try {
    ThisCanThrow();
    void* p = malloc(size);
    if (!p)
      throw "malloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}

void* operator new[](size_t size, align_val_t al) {
  try {
    ThisCanThrow();
    void* p = aligned_alloc(static_cast<size_t>(al), size);
    if (!p)
      throw "aligned_alloc";
    return p;
  }
  catch (...) {
    throw bad_alloc();
  }
}
#endif

int main() {
// Sprawdzamy przykład dołączony do treści zadania z pominięciem wyjątków.
#if TEST_NUM == 101
  binder<string, string> binder1;

  binder1.insert_front("C++ lab2", "C++ lab2 notes");
  binder1.insert_front("C++ lab1", "C++ lab1 notes");
  binder1.insert_after("C++ lab2", "C++ lab3", "C++ lab3 notes");
  binder1.insert_after("C++ lab3", "ASD lab1", "ASD lab1 notes");

  assert(binder1.read("C++ lab1") == "C++ lab1 notes");
  assert(binder1.read("C++ lab2") == "C++ lab2 notes");
  assert(binder1.read("C++ lab3") == "C++ lab3 notes");

  // Należy najpierw usunąć notatkę z tą zakładką.
  binder1.remove("C++ lab2");
  binder1.insert_after("C++ lab1", "C++ lab2", "C++ lab2 notes ver. 2");

  // Używamy read w wersji nie const.
  binder1.read("C++ lab3") += " ver. 2";

  // Wymuszamy użycie read w wersji const.
  string const & str1 = as_const(binder1).read("C++ lab3");
  assert(str1 == "C++ lab3 notes ver. 2");

  string_view notes[] = {"C++ lab1 notes", "C++ lab2 notes ver. 2",
                         "C++ lab3 notes ver. 2", "ASD lab1 notes"};

  size_t n = 0;
  for (auto iter = binder1.cbegin(); iter != binder1.cend(); ++iter)
    assert(*iter == notes[n++]);

  binder1.remove();

  assert(binder1.size() == 3);

  binder<string, pair<string, string>> pair_binder;

  pair_binder.insert_front("kartka 1", make_pair("strona 1", "strona 2"));
  pair_binder.insert_after("kartka 1", "kartka 2", make_pair("strona 3", "strona 4"));

  assert(pair_binder.cbegin()->first == "strona 1");
  assert(pair_binder.cbegin() != pair_binder.cend());

  binder<int, int> int_binder;
  int_binder.insert_front(0, 0);
  vector<binder<int, int>> vec;
  for (int i = 1; i < 100000; i++)
    int_binder.insert_after(i - 1, i, i);
  for (int i = 0; i < 1000000; i++)
    vec.push_back(int_binder);  // Wszystkie obiekty w vec współdzielą dane.
#endif

// Testujemy konstruktor bezparametrowy, konstruktor kopiujący, konstruktor
// przenoszący, operator przypisania.
#if TEST_NUM == 102
  binder<int, int> q;

  q.insert_front(3, 33);
  q.insert_after(3, 4, 44);
  q.insert_front(5, 45);
  q.insert_after(5, 6, 55);

  vector<int> keys = {5, 6, 3, 4};

  binder<int, int> r = q;
  assert(check_with_keys(r, q, keys));

  binder<int, int> b(foo(q));
  assert(check_with_keys(b, q, keys));

  binder<int, int> t;
  t = q;
  assert(check_with_keys(t, q, keys));

  binder<int, int> u(r);
  assert(check_with_keys(u, r, keys));

  binder<int, int> v = binder<int, int>(foo(r));
  assert(check_with_keys(v, r, keys));

  t = r;
  assert(check_with_keys(t, r, keys));

  binder<int, int> w;
  w = move(r);
  assert(check_with_keys(t, w, keys));

  r = foo(w);
  assert(check_with_keys(t, r, keys));
#endif

// Testujemy metody size, insert_front(k, v), insert_after(k1, k2, v), read(),
// read(k), remove(), remove(k), clear.
#if TEST_NUM == 103
  binder<int, int> bi;
  binder<int, int> const &const_bi = bi;
  assert(bi.size() == 0);

  for (int i = 0; i < 1000; i++)
    test_insert_front_spec(bi, i, i);
  assert(*const_bi.cbegin() == 999);
  assert(const_bi.read(999) == 999);
  assert(bi.size() == 1000);

  test_remove_spec(bi, 999);
  assert(*const_bi.cbegin() == 998);
  assert(const_bi.read(998) == 998);
  assert(bi.size() == 999);

  test_remove_key_spec(bi, 0);
  assert(*const_bi.cbegin() == 998);
  assert(const_bi.read(998) == 998);
  assert(bi.size() == 998);

  for (int i = 998; i >= 500; i--) {
    assert(*const_bi.cbegin() == i);
    assert(const_bi.read(i) == i);
    test_remove_spec(bi, i);
    test_remove_key_spec(bi, 999 - i);
  }
  assert(bi.size() == 0);

  for (int k1 = 9; k1 >= 0; k1--) {
    test_insert_front_spec(bi, 100 * k1, 0);
    for (int k2 = 99; k2 > 0; k2--)
      test_insert_after_spec(bi, 100 * k1, 100 * k1 + k2, k2);
  }

  typename binder<int, int>::const_iterator it1 = bi.cbegin();
  for (int k1 = 0; k1 <= 9; k1++) {
    for (int k2 = 0; k2 <= 99; k2++) {
      assert(*(it1++) == k2);
      assert(const_bi.read(100 * k1 + k2) == k2);
    }
  }

  for (int k1 = 1; k1 <= 7 ; k1++) {
    for (int k2 = 2; k2 <= 97; k2++) {
      assert(const_bi.read(100 * k1 + k2) == k2);
      test_remove_key_spec(bi, 100 * k1 + k2);
    }
    test_remove_spec(bi, k1 - 1);
  }

  assert(bi.size() == 321);
  bi.clear();
  assert(bi.size() == 0);
#endif

// Testujemy metody cbegin, cend i iterator.
#if TEST_NUM == 104
  binder<int, int> bi;

  for (int i = 1000; i >= 0; i--) {
  bi.insert_front(10 * i, 10 * i);
    for (int j = 9; j >= 1; j--)
      bi.insert_after(10 * i, 10 * i + j , 10 * i + j);
  }

  int i = 0;
  for (binder<int, int>::const_iterator it = bi.cbegin(); it != bi.cend(); ++it, ++i)
    assert(*it == i);
  assert(i == 10010);

  for (int i = 0; i <= 1000; i++) {
    for (int j = 0; j <= 9; j++) {
      bi.insert_after(10 * i + j, 10010 + 10 * i + j, j);
    }
  }

  i = 0;
  for (binder<int, int>::const_iterator it = bi.cbegin(); it != bi.cend(); it++, ++i)
    assert(i % 2 == 0 ? *it == i / 2 : *it == (i / 2) % 10);
  assert(i == 20020);

  binder<Key, Value> bu;

  for (size_t i = 0; i <= 77; i++)
    bu.insert_front(Key{i}, Value{100 - i});

  size_t j = 0;
  for (binder<Key, Value>::const_iterator it = bu.cbegin(); it != bu.cend(); ++it, ++j)
    assert(it->value == 23 + j);
  assert(j == 78);

  binder<Key, pair<Key, Value>> bv;

  for (size_t i = 0; i <= 88; i++)
    bv.insert_front(Key{i}, make_pair(Key{i}, Value{100 - i}));

  j = 0;
  for (binder<Key, pair<Key, Value>>::const_iterator it = bv.cbegin(); it != bv.cend(); ++it, ++j) {
    assert(it->first.key == 88 - j);
    assert(it->second.value == 12 + j);
  }
  assert(j == 89);
#endif

// Testujemy operację swap.
#if TEST_NUM == 105
  binder<int, int> bi;
  for (int i = 0; i < 1000; i++) {
    bi.insert_front(10 * i, i + 3);
    for (int j = 1; j < 10; j++)
      bi.insert_after(10 * i, 10 * i + j, i + j + 5);
  }
  binder<int, int> bu = binder<int, int>(bi);

  binder<int, int> bv;
  for (int i = 0; i > -1000; i--) {
    bv.insert_front(10 * i, i);
    for (int j = 1; j < 10; j++)
      bv.insert_after(10 * i, 10 * i - j, i);
  }
  binder<int, int> bw = binder<int, int>(bv);

  swap(bi, bv);
  assert(bi == bw && bv == bu);

  swap(bi, bv);
  assert(bi == bu && bv == bw);

  swap(bv, bi);
  assert(bi == bw && bv == bu);

  swap(bi, bv);
  assert(bi == bu && bv == bw);

  swap(bi, bi);
  assert(bi == bu);

  binder<int, int> bx, by;
  swap(bx, by);
  assert(bx.size() == 0 && bx == bx);
#endif

// Testujemy, czy metoda read zwraca poprawną referencję.
#if TEST_NUM == 106
  binder<size_t, Value> bi;

  for (size_t i = 1; i <= 555; i++)
    bi.insert_front(i, Value(i));

  for (size_t i = 1; i <= 555; i++)
    assert(bi.read(i).value == i);

  assert(bi.cbegin()->value == 555);

  for (size_t i = 1; i <= 42; i++)
    bi.insert_after(i, i+600, Value(i + 1000));

  for (size_t i = 1; i <= 555; i++)
    assert(bi.read(i).value == i);

  for (size_t i = 1; i <= 42; i++)
    assert(bi.read(i+600).value == i + 1000);

  assert(bi.cbegin()->value == 555);
#endif

// Testujemy, czy metoda clear pozostawia obiekt w spójnym stanie.
#if TEST_NUM == 107
  binder<int, int> bi;

  bi.insert_front(1, 1);
  bi.insert_front(2, 2);
  bi.insert_after(1, 3, 11);
  bi.insert_after(2, 4, 12);
  bi.insert_front(5, 6);
  bi.insert_after(5, 6, 16);
  bi.clear();

  assert(bi.size() == 0);
  for (int i = 1; i <= 6; ++i)
    assert_not_in(bi, i);

  bi.insert_front(1, 1);
  bi.insert_front(2, 2);
  bi.insert_after(1, 3, 11);
  bi.insert_after(2, 4, 12);
  bi.insert_front(5, 6);
  bi.insert_after(5, 6, 16);
  bi.clear();

  binder<int, int> bu(bi);

  assert(bu.size() == 0);
  for (int i = 1; i <= 6; ++i)
    assert_not_in(bu, i);

  binder<int, int> bv;

  bi.insert_front(1, 1);
  bi.insert_front(2, 2);
  bi.insert_after(1, 3, 11);
  bi.insert_after(2, 4, 12);
  bi.insert_front(5, 6);
  bi.insert_after(5, 6, 16);
  bi.clear();

  bv = bi;

  assert(bv.size() == 0);
  for (int i = 1; i <= 6; ++i)
    assert_not_in(bv, i);

  bi.insert_front(7, 7);
  bi.insert_after(7, 8, 17);
  bi.insert_front(9, 9);
  bi.insert_after(9, 10, 19);
  bi.insert_after(8, 11, 18);
  bi.clear();

  bi.insert_front(12, 12);
  bi.insert_front(13, 12);
  bi.insert_front(14, 12);

  assert(bi.size() == 3);
  for (int i = 7; i <= 11; ++i)
    assert_not_in(bv, i);

  for (int i = 12; i <= 14; ++i)
    assert(as_const(bi).read(i) == 12);

  bi.clear();

  for (int i = 12; i <= 14; ++i)
    assert_not_in(bv, i);
  assert(bi.size() == 0);

  bi.insert_front(15, 15);
  bi.insert_after(15, 16, 15);

  bi.clear();

  int sum = 0;
  for (auto it = bi.cbegin(); it != bi.cend(); ++it)
    ++sum;

  assert(sum == 0);

  sum = 0;
  for (auto it = bi.cbegin(); it != bi.cend(); it++)
    ++sum;

  assert(sum == 0);

  bi.insert_front(1, 1);
  bi.insert_front(2, 2);
  bi.insert_after(1, 3, 11);
  bi.insert_after(2, 4, 12);
  bi.insert_front(6, 6);
  bi.insert_after(6, 7, 16);
  bi.clear();

  binder<int, int> bw(move(bi));

  assert(bw.size() == 0);
  for (int i = 1; i <= 7; ++i)
    assert_not_in(bw, i);
#endif

// Czy iterator spełnia wymagany koncept?
#if TEST_NUM == 108
  int keys[] = {5, 1, 3};
  int values[] = {11, 12, 13};

  binder<int, int> bi;
  for (int i = 0; i < 3; ++i)
    bi.insert_front(keys[i], values[i]);

  static_assert(forward_iterator<binder<int, int>::const_iterator>);

  auto i1 = bi.cbegin();
  static_assert(forward_iterator<decltype(i1)>);
  auto i2 = bi.cend();
  static_assert(forward_iterator<decltype(i2)>);
  auto i3 = ++i1;
  static_assert(forward_iterator<decltype(i3)>);
  auto i4 = i3++;
  static_assert(forward_iterator<decltype(i4)>);
#endif

// Test 109 jest w pliku binder_test_extern.cpp.
#if TEST_NUM == 109
#endif

// Sprawdzamy stan iteratorów po wykonaniu operacji.
#if TEST_NUM == 110
  binder<int, int> bi, bu;
  bi.insert_front(0, 0);
  bi.insert_after(0, 1, 1);
  bi.insert_after(1, 2, 2);
  bi.insert_after(2, 4, 4);

  auto iter = bi.cbegin();

  assert(*(iter++) == 0);
  assert(*(++iter) == 2);

  swap(bi, bu);
  bu.insert_after(2, 3, 3);

  assert(bi.size() == 0);
  assert(bu.size() == 5);
  assert(*iter == 2);
  iter++;
  assert(*iter == 3);

  bu.remove(0);
  assert(bu.size() == 4);
  assert(*iter == 3);
  assert(*(++iter) == 4);
  assert(++iter == bu.cend());
#endif

// Testowanie złóżoności pamięciowej.
// Czy rozwiązanie nie przechowuje zbyt dużo kopii zakładek lub notatek?
#if TEST_NUM == 201
  binder<Key, Value> bi;

  for (size_t i = 0; i < 1000; i++) {
    bi.insert_front(Key(10 * i), Value(i));
    for (size_t j = 1; j < 10; j++)
      bi.insert_after(Key(10 * i), Key(10 * i + j), Value(i));
  }

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(bi.size() == 10000);
  assert(Value::instance_count == 10000);
  assert(Key::instance_count == 10000);

  bi.insert_front(Key(10001), Value(10001));

  assert(bi.size() == 10001);
  assert(Value::instance_count == 10001);
  assert(Key::instance_count == 10001);

  bi.insert_after(Key(0), Key(777777), Value(0));

  assert(bi.size() == 10002);
  assert(Value::instance_count == 10002);
  assert(Key::instance_count == 10002);

  bi.remove(Key(777));

  assert(bi.size() == 10001);
  assert(Value::instance_count == 10001);
  assert(Key::instance_count == 10001);

  bi.remove();

  assert(bi.size() == 10000);
  assert(Value::instance_count == 10000);
  assert(Key::instance_count == 10000);
#endif

// Testowanie złóżoności czasowej.
// Czy rozwiązanie nie wykonuje zbyt wielu operacji?
#if TEST_NUM == 202
  do {
    binder<Key, Value> bi;

    for (size_t i = 0; i < 1000; i++) {
      bi.insert_front(Key(10 * i), Value(i));
      for (size_t j = 1; j < 10; j++)
        bi.insert_after(Key(10 * i), Key(10 * i + j), Value(i));
    }

    Key::operation_count = 0;
    Value::operation_count = 0;

    binder<Key, Value> bi_copy(bi);

    clog << "Liczba operacji wykonanych przez konstruktor kopiujący: "
         << Key::operation_count << ", " << Value::operation_count << "\n";

    assert(Key::operation_count == 0 && Value::operation_count == 0);
    assert(bi_copy.size() == 10000);
  } while (0);

  do {
    binder<Key, Value> bi;

    for (size_t i = 0; i < 1000; i++) {
      bi.insert_front(Key(10 * i), Value(i));
      for (size_t j = 1; j < 10; j++)
        bi.insert_after(Key(10 * i), Key(10 * i + j), Value(i));
    }

    Key::operation_count = 0;
    Value::operation_count = 0;

    binder<Key, Value> bi_copy(move(bi));

    clog << "Liczba operacji wykonanych przez konstruktor przenoszący: "
         << Key::operation_count << ", " << Value::operation_count << "\n";

    assert(Key::operation_count == 0 && Value::operation_count == 0);
    assert(bi_copy.size() == 10000);
  } while (0);

  do {
    binder<Key, Value> bi;

    for (size_t i = 0; i < 1000; i++) {
      bi.insert_front(Key(10 * i), Value(i));
      for (size_t j = 1; j < 10; j++)
        bi.insert_after(Key(10 * i), Key(10 * i + j), Value(i));
    }

    Key::operation_count = 0;
    Value::operation_count = 0;

    binder<Key, Value> bi_copy;

    bi_copy = bi;

    clog << "Liczba operacji wykonanych przez operator przypisania: "
         << Key::operation_count << ", " << Value::operation_count << "\n";

    assert(Key::operation_count == 0 && Value::operation_count == 0);
    assert(bi_copy.size() == 10000);
  } while (0);

  Key::operation_count = 0;
  Value::operation_count = 0;

  binder<Key, Value> bi;

  clog << "Liczba operacji wykonanych przez konstruktor bezparametrowy: "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count == 0 && Value::operation_count == 0);

  for (size_t i = 0; i < 1000; i++) {
    bi.insert_front(Key(10 * i), Value(i));
    for (size_t j = 1; j < 10; j++)
      bi.insert_after(Key(10 * i), Key(10 * i + j), Value(i));
  }

  assert(bi.size() == 10000);

  Key::operation_count = 0;
  Value::operation_count = 0;

  bi.insert_front(Key(10001), Value(0));

  clog << "Liczba operacji wykonanych przez insert_front: "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count <= 120 && Value::operation_count <= 15);

  Key::operation_count = 0;
  Value::operation_count = 0;

  bi.insert_after(Key(777), Key(777777), Value(0));

  clog << "Liczba operacji wykonanych przez insert_after: "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count <= 180 && Value::operation_count <= 15);

  Key::operation_count = 0;
  Value::operation_count = 0;

  bi.remove(Key(777));

  clog << "Liczba operacji wykonanych przez remove(k): "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count <= 60 && Value::operation_count <= 15);

  Key::operation_count = 0;
  Value::operation_count = 0;

  bi.remove();

  clog << "Liczba operacji wykonanych przez remove: "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count <= 60 && Value::operation_count <= 15);

  Key::operation_count = 0;
  Value::operation_count = 0;

  [[maybe_unused]] auto v2 = bi.read(Key(555));

  clog << "Liczba operacji wykonanych przez read(k): "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count <= 60 && Value::operation_count <= 15);

  do {
    binder<Key, Value> const bi_copy(bi);

    Key::operation_count = 0;
    Value::operation_count = 0;

    [[maybe_unused]] auto v = bi_copy.read(Key(444));

    clog << "Liczba operacji wykonanych przez read(k) const: "
         << Key::operation_count << ", " << Value::operation_count << "\n";

    assert(Key::operation_count <= 60 && Value::operation_count <= 15);
  } while (0);


  Key::operation_count = 0;
  Value::operation_count = 0;

  [[maybe_unused]] auto size = bi.size();

  clog << "Liczba operacji wykonanych przez size: "
       << Key::operation_count << ", " << Value::operation_count << "\n";

  assert(Key::operation_count == 0 && Value::operation_count == 0);
#endif

// Czy podnoszone są wyjątki zgodnie ze specyfikacją?
// remove
#if TEST_NUM == 301
  binder<int, int> b;

  b.insert_front(1, 1);
  b.clear();

  bool exception_catched = false;
  try {
    b.remove();
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// remove(k)
#if TEST_NUM == 302
  binder<int, int> b;

  b.insert_front(1, 1);

  bool exception_catched = false;
  try {
    b.remove(2);
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// read(k)
#if TEST_NUM == 303
  binder<int, int> b;

  b.insert_front(1, 1);

  bool exception_catched = false;
  try {
    [[maybe_unused]] auto v = b.read(2);
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// insert_front(k, v)
#if TEST_NUM == 304
  binder<int, int> b;

  b.insert_front(1, 1);

  bool exception_catched = false;
  try {
    b.insert_front(1, 2);
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// insert_after(k1, k2, v) - nieistniejący k1
#if TEST_NUM == 305
  binder<int, int> b;

  b.insert_front(1, 1);

  bool exception_catched = false;
  try {
    b.insert_after(2, 3, 3);
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// insert_after(k1, k2, v) - zajęty k2
#if TEST_NUM == 306
  binder<int, int> b;

  b.insert_front(1, 1);
  b.insert_front(2, 2);

  bool exception_catched = false;
  try {
    b.insert_after(1, 2, 3);
  }
  catch (invalid_argument const &) {
    exception_catched = true;
  }
  catch (...) {
  }
  assert(exception_catched);
#endif

// Czy metoda size nie podnosi wyjątku?
#if TEST_NUM == 401
  binder<Key, Value> b;

  b.insert_front(Key(1), Value(11));
  b.insert_front(Key(2), Value(12));
  b.insert_front(Key(3), Value(13));
  b.insert_after(Key(1), Key(4), Value(21));
  b.insert_after(Key(2), Key(5), Value(22));
  b.insert_after(Key(3), Key(6), Value(23));

  size_t size;

  NoThrowCheck(b, [&](auto const &b) {size = b.size();}, "size");
  assert(size == 6);
#endif

// Czy konstruktor przenoszący nie podnosi wyjątku?
#if TEST_NUM == 402
  binder<Key, Value> b1, b2;

  for (size_t i = 1; i <= 7; i++) {
    b1.insert_front(Key(20 * i), Value(i));
    for (size_t j = 1; j <= 11; j++)
      b1.insert_after(Key(20 * i), Key(20 * i + j), Value(i));
  }

  size_t size = b1.size();

  binder<Key, Value> b1_copy(b1);

  NoThrowCheck(b1, [&b2](auto &b) {b2 = move(b);}, "move");
  assert(b1_copy == b2);
  assert(b2.size() == size);
#endif

// Czy operacja swap nie podnosi wyjątku?
#if TEST_NUM == 403
  binder<Key, Value> b1, b2;

  for (size_t i = 1; i <= 8; i++) {
    b1.insert_front(Key(17 * i), Value(i));
    for (size_t j = 1; j <= 9; j++)
      b1.insert_after(Key(17 * i), Key(18 * i + j), Value(i));
  }

  size_t size = b1.size();

  binder<Key, Value> b1_copy(b1);
  binder<Key, Value> b2_copy(b2);

  auto f = [&b2](auto &b) {swap(b, b2);};

  NoThrowCheck(b1, f, "swap");
  assert(b1_copy == b2);
  assert(b2_copy == b1);
  assert(b2.size() == size);

  NoThrowCheck(b1, f, "swap");
  assert(b1_copy == b1);
  assert(b2_copy == b2);
  assert(b1.size() == size);
#endif

// Czy destruktor nie podnosi wyjątku?
#if TEST_NUM == 404
  char buf[sizeof (binder<Key, Value>)];
  binder<Key, Value> *b = new (buf) binder<Key, Value>();

  for (int i = 1; i <= 9; i++) {
    b->insert_front(Key(30 * i), Value(i));
    for (int j = 1; j <= 9; j++)
      b->insert_after(Key(30 * i), Key(32 * i + j), Value(i));
  }

  NoThrowCheck(*b, [](auto &b) {b.~binder<Key, Value>();}, "destruktor");
#endif

// Czy iterator nie podnosi wyjątku?
#if TEST_NUM == 405
  binder<Key, Value> b;

  for (int i = 1; i <= 4; i++) {
    b.insert_front(Key(16 * i), Value(7*i));
    for (int j = 1; j <= 15; j++)
      b.insert_after(Key(16 * i), Key(16 * i + j), Value(12*i));
  }

  size_t sum1 = 0;
  auto f1 = [&](auto &b) {for (auto it = b.cbegin(); it != b.cend(); ++it) sum1 += it->value;};
  NoThrowCheck(b, f1, "iterator");
  assert(sum1 == 1870);

  size_t sum2 = 0;
  auto f2 = [&](auto &b) {for (auto it = b.cbegin(); it != b.cend(); it++) sum2 += it->value;};
  NoThrowCheck(b, f2, "iterator");
  assert(sum2 == 1870);
#endif

// Czy metoda clear nie podnosi wyjątku?
#if TEST_NUM == 406
  binder<Key, Value> b;

  b.insert_front(Key(1), Value(11));
  b.insert_front(Key(2), Value(12));
  b.insert_front(Key(3), Value(13));
  b.insert_after(Key(1), Key(66), Value(21));
  b.insert_after(Key(2), Key(555), Value(22));
  b.insert_after(Key(3), Key(4444), Value(23));

  NoThrowCheck(b, [](auto &b) {b.clear();}, "clear");
  assert(b.size() == 0);
#endif

// Czy metoda clear nie podnosi wyjątku?
#if TEST_NUM == 407
  binder<Key, Value> b1;

  b1.insert_front(Key(1), Value(11));
  b1.insert_front(Key(2), Value(12));
  b1.insert_front(Key(3), Value(13));
  b1.insert_after(Key(1), Key(66), Value(21));
  b1.insert_after(Key(2), Key(555), Value(22));
  b1.insert_after(Key(3), Key(4444), Value(23));

  binder<Key, Value> b2(b1);

  NoThrowCheck(b1, [](auto &b) {b.clear();}, "clear");
  assert(b1.size() == 0);
  assert(b2.size() == 6);
#endif

// Testujemy silne gwarancje. Te testy sprawdzają też przeźroczystość na wyjątki.
// insert_front
#if TEST_NUM == 501
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;

    Key k1(99), k2(0);
    Value v1(9999), v2(100);
    b.insert_front(k1, v1);
    d.insert_front(k1, v1);

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &b) {b.insert_front(k2, v2);}, "insert_front");
    if (result) {
      assert(b.read(k2) == v2);
      d.insert_front(k2, v2);
    }
    success &= result;

    for (size_t i = 100; i < 110; i++) {
      Key k3(i);
      Value v3(100);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &b) {b.insert_front(k3, v3);}, "insert_front");
      if (result) {
        assert(b.read(k3) == v3);
        d.insert_front(k3, v3);
      }
      success &= result;
    }

    for (size_t i = 0; i < 10; i++) {
      Key k3((48271 * i) % 31 + 200);
      Value v3(100);
      b.insert_front(k3, v3);
      d.insert_front(k3, v3);
    }

    for (size_t i = 0; i < 10; i++) {
      Key k3((16807 * i) % 31 + 300);
      Value v3(100);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &b) {b.insert_front(k3, v3);}, "insert_front");
      if (result) {
        assert(b.read(k3)== v3);
        d.insert_front(k3, v3);
      }
      success &= result;
    }

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla insert_front: " << trials << "\n";
#endif

// insert_after
#if TEST_NUM == 502
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;

    Key k1(99), k2(0);
    Value v1(9999), v2(100);
    b.insert_front(k1, v1);
    d.insert_front(k1, v1);

    throw_countdown = trials;
    bool result = StrongCheck(b, d, [&](auto &b) {b.insert_after(k1, k2, v2);}, "insert_after");
    if (result) {
      assert(b.read(k2) == v2);
      d.insert_after(k1, k2, v2);
    }
    success &= result;

    for (size_t i = 100; i < 110; i++) {
      Key k3(i);
      Value v3(100);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &b) {b.insert_after(k2, k3, v3);}, "insert_after");
      if (result) {
        assert(b.read(k3) == v3);
        d.insert_after(k2, k3, v3);
      }
      success &= result;
    }

    for (size_t i = 0; i < 10; i++) {
      Key k3((48271 * i) % 31 + 200);
      Value v3(100);
      b.insert_after(k1, k3, v3);
      d.insert_after(k1, k3, v3);
    }

    for (size_t i = 0; i < 10; i++) {
      Key k3((16807 * i) % 31 + 300);
      Value v3(100);

      throw_countdown = trials;
      result = StrongCheck(b, d, [&](auto &b) {b.insert_after(k2, k3, v3);}, "insert_after");
      if (result) {
        assert(b.read(k3)== v3);
        d.insert_after(k2, k3, v3);
      }
      success &= result;
    }

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla insert_after: " << trials << "\n";
#endif

// remove
#if TEST_NUM == 503
  bool success = false, result;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;

    Key k1(1), k2(22), k3(333), k4(4444);
    Value v1(41), v2(42);

    b.insert_front(k1, v1);
    b.insert_front(k2, v1);
    b.insert_after(k1, k3, v2);
    b.insert_after(k2, k4, v2);

    d.insert_front(k1, v1);
    d.insert_front(k2, v1);
    d.insert_after(k1, k3, v2);
    d.insert_after(k2, k4, v2);

    throw_countdown = trials;

    for (int i = 0; i < 4; i++) {
      result = StrongCheck(b, d, [](auto &b) {b.remove();}, "remove");
      if (result)
        d.remove();
      success &= result;
    }

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla remove: " << trials << "\n";
#endif

// remove(k)
#if TEST_NUM == 504
  bool success = false, result;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;
    int int_arr[9] = {0, 1, 22, 333, 4444, 5, 6, 77, 88};
    vector<Key> k;
    for (int i = 0; i < 9; ++i) {
          k.push_back(Key(int_arr[i]));
    }

    Value v1(41), v2(42);

    b.insert_front(k[1], v1);
    b.insert_after(k[1], k[3], v1);
    b.insert_front(k[2], v1);
    b.insert_after(k[2], k[4], v1);
    b.insert_after(k[1], k[5], v2);
    b.insert_after(k[1], k[6], v2);
    b.insert_after(k[2], k[7], v2);
    b.insert_after(k[2], k[8], v2);

    d.insert_front(k[1], v1);
    d.insert_after(k[1], k[3], v1);
    d.insert_front(k[2], v1);
    d.insert_after(k[2], k[4], v1);
    d.insert_after(k[1], k[5], v2);
    d.insert_after(k[1], k[6], v2);
    d.insert_after(k[2], k[7], v2);
    d.insert_after(k[2], k[8], v2);

    throw_countdown = trials;

    for (int i = 0; i < 2; i++) {
      result = StrongCheck(b, d, [&](auto &b) {b.remove(k[2 * i + 1]);}, "remove(k)");
      if (result)
        d.remove(k[2 * i + 1]);
      success &= result;
    }

    assert(b == d);

    for (int i = 0; i < 2; i++) {
      result = StrongCheck(b, d, [&](auto &b) {b.remove(k[2 * i + 2]);}, "remove(k)");
      if (result)
        d.remove(k[2* i + 2]);
      success &= result;
    }

    assert(b == d);

    for (int i = 0; i < 2; i++) {
      result = StrongCheck(b, d, [&](auto &b) {b.remove(k[2 * i + 5]);}, "remove(k)");
      if (result)
        d.remove(k[2 * i + 5]);
      success &= result;
    }

    assert(b == d);

    for (int i = 0; i < 2; i++) {
      result = StrongCheck(b, d, [&](auto &b) {b.remove(k[2 * i + 6]);}, "remove(k)");
      if (result)
        d.remove(k[2 * i + 6]);
      success &= result;
    }

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla remove(k): " << trials << "\n";
#endif

// read(k)
#if TEST_NUM == 505
  bool success = false, result, op_result;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;

    Key k1(1), k2(2), k3(3), k4(4);
    Value v1(41), v2(42);

    b.insert_front(k1, v1);
    b.insert_front(k4, v2);
    b.insert_after(k1, k3, v1);
    b.insert_after(k4, k2, v2);

    d.insert_front(k1, v1);
    d.insert_front(k4, v2);
    d.insert_after(k1, k3, v1);
    d.insert_after(k4, k2, v2);

    throw_countdown = trials;

    result = StrongCheck(b, d, [&](auto &b) {op_result = b.read(k1) == v1;}, "read(k)");
    if (result)
      assert(op_result);
    success &= result;

    result = StrongCheck(b, d, [&](auto &b) {op_result = b.read(k2) == v2;}, "read(k)");
    if (result)
      assert(op_result);
    success &= result;

    result = StrongCheck(b, d, [&](auto const &b) {op_result = b.read(k1) == v1;}, "read(k)");
    if (result)
      assert(op_result);
    success &= result;

    result = StrongCheck(b, d, [&](auto const &b) {op_result = b.read(k2) == v2;}, "read(k)");
    if (result)
      assert(op_result);
    success &= result;

    assert(b == d);
  }

  clog << "Liczba prób wykonanych dla read(k): " << trials << "\n";
#endif

// clear
#if TEST_NUM == 506
  bool success = false, result;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d;

    Key k1(1), k2(2), k3(3), k4(4), k5(5);
    Value v1(41), v2(42);

    b.insert_front(k1, v1);
    b.insert_after(k1, k2, v2);
    b.insert_front(k3, v1);
    b.insert_after(k2, k4, v2);
    b.insert_after(k3, k5, v2);

    throw_countdown = trials;

    result = StrongCheck(b, d, [](auto &b) {b.clear();}, "clear");
    if (result)
      assert(b == d);
    success &= result;
  }

  clog << "Liczba prób wykonanych dla clear: " << trials << "\n";
#endif

// przypisanie
#if TEST_NUM == 507
  bool success = false, result;
  int trials;

  for (trials = 1; !success; trials++) {
    success = true;

    binder<Key, Value> b, d, u;

    Key k1(1), k2(2), k3(3), k4(4), k5(5);
    Value v1(41), v2(42);

    b.insert_front(k1, v1);
    b.insert_after(k1, k2, v2);
    b.insert_front(k3, v1);
    b.insert_after(k2, k4, v2);
    b.insert_after(k3, k5, v2);
    [[maybe_unused]] auto p = b.read(k4); // Wymuszamy wykonanie kopii.

    d.insert_front(k1, v1);
    d.insert_after(k1, k2, v2);
    d.insert_front(k3, v1);
    d.insert_after(k2, k4, v2);
    d.insert_after(k3, k5, v2);

    throw_countdown = trials;

    result = StrongCheck(b, d, [&](auto &b) {u = b;}, "operator przypisania");
    if (result)
      assert(u == d);
    success &= result;
  }

  clog << "Liczba prób wykonanych dla operatora przypisania: " << trials << "\n";
#endif

// konstruktor kopiujący
#if TEST_NUM == 508
  bool success = false;
  int trials;

  for (trials = 1; !success; trials++) {
    binder<Key, Value> b, d;

    Key k1(1), k2(2), k3(3), k4(4), k5(5);
    vector<Key> kv = {k2, k5, k4, k1, k3};
    Value v1(41), v2(42);

    b.insert_front(k1, v1);
    b.insert_front(k2, v2);
    b.insert_after(k1, k3, v1);
    b.insert_after(k2, k4, v2);
    b.insert_after(k2, k5, v2);
    [[maybe_unused]] auto p = b.read(k2); // Wymuszamy wykonanie kopii.

    d.insert_front(k1, v1);
    d.insert_front(k2, v2);
    d.insert_after(k1, k3, v1);
    d.insert_after(k2, k4, v2);
    d.insert_after(k2, k5, v2);

    throw_countdown = trials;
    try {
      throw_checking = true;
      binder<Key, Value> u(b);
      throw_checking = false;
      success = true;
      assert(check_with_keys(u, d, kv));
    }
    catch (...) {
      throw_checking = false;
      bool unchanged = check_with_keys(d, b, kv);
      if (!unchanged) {
        clog << "Konstruktor kopiujący nie daje silnej gwarancji\n.";
        assert(unchanged);
      }
    }
  }

  clog << "Liczba prób wykonanych dla konstruktora kopiującego: " << trials << "\n";
#endif

// Testujemy ważność iteratora po zgłoszeniu wyjątku (insert_front).
#if TEST_NUM == 509
  int trials;
  bool success = false;

  for (trials = 1; !success; trials++) {
    binder<Key, Value> b1;

    b1.insert_front(Key(0), Value(10));
    b1.insert_after(Key(0), Key(2), Value(12));
    b1.insert_after(Key(0), Key(1), Value(11));

    auto b2 = make_unique<binder<Key, Value>>(b1);

    b2->insert_after(Key(2), Key(3), Value(13));

    binder<Key, Value> b3 = *b2;

    auto it = b3.cbegin();
    auto end = b3.cend();
    Key key1(5);
    Value value1(15);

    throw_countdown = trials;
    try {
      throw_checking = true;
      b3.insert_front(key1, value1);
      throw_checking = false;
      break; // Jeśli się udało, to poniżej będą problemy.
    }
    catch (...) {
      throw_checking = false;
    }
    b2.reset();
    for (int i = 10; it != end; ++it, ++i)
      assert(*it == Value(i));
  }

  clog << "Liczba wykonanych prób: " << trials << "\n";
#endif

// Testujemy ważność iteratora po zgłoszeniu wyjątku (insert_after).
#if TEST_NUM == 510
  int trials;
  bool success = false;

  for (trials = 1; !success; trials++) {
    binder<Key, Value> b1;

    b1.insert_front(Key(0), Value(10));
    b1.insert_after(Key(0), Key(2), Value(12));
    b1.insert_after(Key(0), Key(1), Value(11));

    auto b2 = make_unique<binder<Key, Value>>(b1);

    b2->insert_after(Key(2), Key(3), Value(13));

    binder<Key, Value> b3 = *b2;

    auto it = b3.cbegin();
    auto end = b3.cend();
    Key key1(5);
    Value value1(15);

    throw_countdown = trials;
    try {
      throw_checking = true;
      b3.insert_after(Key(1), key1, value1);
      throw_checking = false;
      break; // Jeśli się udało, to poniżej będą problemy.
    }
    catch (...) {
      throw_checking = false;
    }
    b2.reset();
    for (int i = 10; it != end; ++it, ++i)
      assert(*it == Value(i));
  }

  clog << "Liczba wykonanych prób: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku (insert_front).
#if TEST_NUM == 511
  bool success = false;
  int trials;

  binder<Key, Value> b1;
  b1.insert_front(Key(1), Value(1));
  binder<Key, Value> b2(b1);

  assert(Key::instance_count == 1);
  assert(Value::instance_count == 1);

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      b2.insert_front(Key(2), Value(2));
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
    }

    if (success) {
      assert(Key::instance_count == 3 || Key::instance_count == 2);
      assert(Value::instance_count == 3);
    }
    else {
      assert(Key::instance_count == 1);
      assert(Value::instance_count == 1);
    }
  }

  clog << "Liczba wykonanych prób: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku (remove).
#if TEST_NUM == 512
  bool success = false;
  int trials;

  binder<Key, Value> b1;
  b1.insert_front(Key(1), Value(1));
  b1.insert_front(Key(2), Value(2));
  binder<Key, Value> b2(b1);

  assert(Key::instance_count == 2);
  assert(Value::instance_count == 2);

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      b2.remove();
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
    }

    if (success) {
      assert(Key::instance_count == 3 || Key::instance_count == 2);
      assert(Value::instance_count == 3);
    }
    else {
      assert(Key::instance_count == 2);
      assert(Value::instance_count == 2);
    }
  }

  clog << "Liczba wykonanych prób: " << trials << "\n";
#endif

// Testujemy przywracanie stanu po zgłoszeniu wyjątku (insert_after).
#if TEST_NUM == 513
  bool success = false;
  int trials;

  binder<Key, Value> b1;
  b1.insert_front(Key(1), Value(1));
  b1.insert_front(Key(2), Value(2));
  binder<Key, Value> b2(b1);

  assert(Key::instance_count == 2);
  assert(Value::instance_count == 2);

  for (trials = 1; !success; trials++) {
    throw_countdown = trials;

    try {
      throw_checking = true;
      b2.remove(Key(1));
      throw_checking = false;
      success = true;
    }
    catch (...) {
      throw_checking = false;
      success = false;
    }

    if (success) {
      assert(Key::instance_count == 3 || Key::instance_count == 2);
      assert(Value::instance_count == 3);
    }
    else {
      assert(Key::instance_count == 2);
      assert(Value::instance_count == 2);
    }
  }

  clog << "Liczba wykonanych prób: " << trials << "\n";
#endif

// Testujemy kopiowanie przy modyfikowaniu.
#if TEST_NUM == 601
  binder<Key, Value> b1;

  for (int i = 1; i <= 7; i++) {
    b1.insert_front(Key(i), Value(0));
    for (int j = 1; j <= 9; j++) {
      b1.insert_after(Key(i), Key(i + 20 * j), Value(j));
      b1.insert_front(Key(j + 20 * i + 10), Value(i));
    }
  }

  size_t k_count = Key::instance_count;
  assert(k_count == 133);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Value::instance_count == k_count);

  binder<Key, Value> b2(b1);
  binder<Key, Value> b3;
  binder<Key, Value> b4;
  b3 = b1;
  b4 = move(b2);

  assert(b1.size() == k_count);
  assert(b3.size() == k_count);
  assert(b4.size() == k_count);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == k_count);
  assert(Value::instance_count == k_count);

  assert(b3.read(Key(159)).value == 7);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 2 * k_count || Key::instance_count == k_count);
  assert(Value::instance_count == 2 * k_count);

  assert(b4.read(Key(187)).value == 9);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 3 * k_count || Key::instance_count == k_count);
  assert(Value::instance_count == 3 * k_count);

  b1.remove();
  b1.insert_front(Key(13), Value(17));
  b1.remove(Key(75));

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 3 * k_count - 1 || Key::instance_count == k_count + 1);
  assert(Value::instance_count == 3 * k_count - 1);

  binder<Key, Value> b5(b1);
  binder<Key, Value> b6;
  binder<Key, Value> b7;
  b6 = b1;
  b7 = move(b5);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 3 * k_count - 1 || Key::instance_count == k_count + 1);
  assert(Value::instance_count == 3 * k_count - 1);

  b1.remove();
  b1.insert_after(Key(21), Key(15), Value(18));
  b1.remove(Key(57));

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 4 * k_count - 3 || Key::instance_count == k_count + 2);
  assert(Value::instance_count == 4 * k_count - 3);

  b7.clear();

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 4 * k_count - 3 || Key::instance_count == k_count + 2);
  assert(Value::instance_count == 4 * k_count - 3);

  b1.clear();

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 3 * k_count - 1 || Key::instance_count == k_count + 1);
  assert(Value::instance_count == 3 * k_count - 1);
#endif

// Testujemy unieważnianie udzielonych referencji.
#if TEST_NUM == 602
  binder<Key, Value> b1;

  for (int i = 1; i <= 3; i++) {
    b1.insert_front(Key(i), Value(0));
    for (int j = 1; j <= 3; j++) {
      b1.insert_after(Key(i), Key(i + 10 * j), Value(j));
      b1.insert_front(Key(j + 10 * i + 5), Value(i));
    }
  }

  size_t k_count = Key::instance_count;
  assert(k_count == 21);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Value::instance_count == k_count);
  assert(b1.read(Key(38)).value == 3);

  b1.read(Key(38)).value = 333;

  assert(b1.read(Key(38)).value == 333);

  b1.remove(Key(38));

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == k_count - 1);
  assert(Value::instance_count == k_count - 1);

  binder<Key, Value> b2(b1);
  binder<Key, Value> b3;
  binder<Key, Value> b4;
  b3 = b1;
  b4 = move(b2);

  assert(b1.size() == k_count - 1);
  assert(b3.size() == k_count - 1);
  assert(b4.size() == k_count - 1);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == k_count - 1);
  assert(Value::instance_count == k_count -1);

  assert(b1.read(Key(2)).value == 0);

  b1.read(Key(2)).value = 33;

  assert(b1.read(Key(2)).value == 33);
  assert(Key::instance_count == 2 * k_count - 2 || Key::instance_count == k_count - 1);
  assert(Value::instance_count == 2 * k_count - 2);

  b1.remove(Key(2));

  assert(b1.size() == k_count - 2);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 2 * k_count - 3 || Key::instance_count == k_count - 1);
  assert(Value::instance_count == 2 * k_count - 3);

  binder<Key, Value> b5(b1);
  binder<Key, Value> b6;
  binder<Key, Value> b7;
  binder<Key, Value> b8;
  b6 = b1;
  b7 = move(b5);
  b8 = b4;

  assert(b6.size() == k_count - 2);
  assert(b7.size() == k_count - 2);
  assert(b8.size() == k_count - 1);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';
  clog << "Liczba przechowywanych notatek: " << Value::instance_count << '\n';

  assert(Key::instance_count == 2 * k_count - 3 || Key::instance_count == k_count - 1);
  assert(Value::instance_count == 2 * k_count - 3);
#endif

#if TEST_NUM == 603
  auto bi = make_unique<binder<int, int>>();
  vector<binder<int, int>> vec;
  for (int i = 0; i < 66666; i++)
    bi->insert_front(i, i);
  for (int i = 0; i < 77777; i++)
    vec.push_back(*bi);
  bi.reset();
  for (int i = 0; i < 10; i++)
    vec[i].insert_after(i+55555, i+88888, i);
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 11; j++) {
      if (i != j) {
        assert(!(vec[i] == vec[j]));
      }
    }
  }
  assert(!(vec[0] == vec[77776]));
  assert(vec[11] == vec[33332]);
#endif

// Testujemy rozdzielanie stosów przy modyfikowaniu notatek.
#if TEST_NUM == 604
  size_t k_count;

  do {
    binder<Key, long> b1;

    b1.insert_front(Key(101), 111);
    b1.insert_front(Key(102), 122);
    b1.insert_after(Key(101), Key(201), 121);
    b1.insert_front(Key(103), 113);
    b1.insert_after(Key(103), Key(203), 123);
    b1.insert_after(Key(102), Key(202), 112);
    b1.insert_after(Key(203), Key(303), 133);

    k_count = Key::instance_count;
    assert(k_count == 7);

    clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';

    binder<Key, long> b2(b1);

    clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';

    assert(Key::instance_count == k_count);

    long &v1 = b1.read(Key(303));
    v1 = 233;

    clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';

    assert(Key::instance_count == 2 * k_count || Key::instance_count == k_count);

    assert(b1.read(Key(303)) == 233);
    assert(b2.read(Key(303)) == 133);
  } while (0);

  clog << "Liczba przechowywanych zakładek: " << Key::instance_count << '\n';

  assert(Key::instance_count == 0);
#endif
}

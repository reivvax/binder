#include "binder.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string>
#include <string_view>

using std::as_const;
using std::invalid_argument;
using std::pair;
using std::make_pair;
using std::string;
using std::string_view;
using std::vector;
using cxx::binder;

int main() {
  binder<string, string> binder1;

  binder1.insert_front("C++ lab2", "C++ lab2 notes");
  binder1.insert_front("C++ lab1", "C++ lab1 notes");
  binder1.insert_after("C++ lab2", "C++ lab3", "C++ lab3 notes");
  binder1.insert_after("C++ lab3", "ASD lab1", "ASD lab1 notes");

  assert(binder1.read("C++ lab1") == "C++ lab1 notes");
  assert(binder1.read("C++ lab2") == "C++ lab2 notes");
  assert(binder1.read("C++ lab3") == "C++ lab3 notes");

  bool catched = false;
  try {
    // Próba użycia istniejącej już zakładki powoduje wyjątek.
    binder1.insert_after("C++ lab1", "C++ lab2", "C++ lab2 notes ver. 2");
    assert(0);
  }
  catch (invalid_argument const &) {
    catched = true;
  }
  catch (...) {
    assert(0);
  }
  assert(catched);

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
}

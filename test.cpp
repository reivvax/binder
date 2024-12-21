#include "binder.h"

using cxx::binder;

int main() {
    binder<int, int> binder1;
    binder1.insert_front(1, 1);
    binder1.insert_after(1, 2, 2);

    
    binder1._print();
}
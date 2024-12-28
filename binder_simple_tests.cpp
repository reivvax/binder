#include <iostream>
#include <stdexcept>
#include <string>
#include "binder.h" // Załóżmy, że tu jest zaimplementowana klasa binder
#include <vector>

// Pomocnicza funkcja testująca - raportuje wynik
void assert_true(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "PASS: " << test_name << std::endl;
    } else {
        std::cout << "FAIL: " << test_name << std::endl;
    }
}

void simple_tests() {
    cxx::binder<int, int> b0;

    b0.insert_front(10, -5);
    b0.insert_front(3, 2);
    assert_true(b0.size() == 2, "Insert front increases size");

    cxx::binder<int, int> b1(b0);

    int const& v0 = b1.read(10);
    assert_true(v0 == -5, "Read returns correct value");
    
    b1.insert_front(5, 4);
    assert_true(b1.size() == 3, "Insert front increases size");

    b1.remove();

    assert_true(b1.size() == 2, "Remove decreases size");
    
    int const& v1 = b1.read(10);
    assert_true(v1 == -5, "Read returns correct value after remove");

    cxx::binder<int, int> b2(std::move(b1));

    int const& v2 = b2.read(10);
    assert_true(v2 == -5, "Read returns correct value after move");
}

// Testy klasy binder
void gpt_tests() {
    // Test 1: Konstruktor bezparametrowy
    {
        cxx::binder<std::string, std::string> b;
        assert_true(b.size() == 0, "Default constructor creates empty binder");
    }

    // Test 2: Dodawanie notatek na początek
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        assert_true(b.size() == 1, "Insert front increases size");
        assert_true(b.read("A") == "Note A", "Insert front stores correct value");
    }

    // Test 3: Wyjątek przy dodawaniu duplikatu
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        try {
            b.insert_front("A", "Duplicate Note A");
            assert_true(false, "Insert duplicate throws exception");
        } catch (const std::invalid_argument&) {
            assert_true(true, "Insert duplicate throws exception");
        }
    }

    // Test 4: Wstawianie po danej zakładce
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        b.insert_after("A", "B", "Note B");
        assert_true(b.size() == 2, "Insert after increases size");
        assert_true(b.read("B") == "Note B", "Insert after stores correct value");
    }

    // Test 5: Wyjątek, gdy nie ma zakładki prev_k w insert_after
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        try {
            b.insert_after("NonExistent", "B", "Note B");
            assert_true(false, "Insert after invalid key throws exception");
        } catch (const std::invalid_argument&) {
            assert_true(true, "Insert after invalid key throws exception");
        }
    }

    // Test 6: Usuwanie pierwszej notatki
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        b.remove();
        assert_true(b.size() == 0, "Remove first note decreases size");
    }

    // Test 7: Wyjątek przy usuwaniu z pustego skoroszytu
    {
        cxx::binder<std::string, std::string> b;
        try {
            b.remove();
            assert_true(false, "Remove on empty binder throws exception");
        } catch (const std::invalid_argument&) {
            assert_true(true, "Remove on empty binder throws exception");
        }
    }

    // Test 8: Usuwanie po kluczu
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        b.remove("A");
        assert_true(b.size() == 0, "Remove by key decreases size");
    }

    // Test 9: Wyjątek przy usuwaniu nieistniejącej zakładki
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        try {
            b.remove("NonExistent");
            assert_true(false, "Remove non-existent key throws exception");
        } catch (const std::invalid_argument&) {
            assert_true(true, "Remove non-existent key throws exception");
        }
    }

    // Test 10: Modyfikacja notatek przez read
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        b.read("A") = "Modified Note A";
        assert_true(b.read("A") == "Modified Note A", "Read allows modification");
    }

    // Test 11: Clear
    {
        cxx::binder<std::string, std::string> b;
        b.insert_front("A", "Note A");
        b.insert_front("B", "Note B");
        b.clear();
        assert_true(b.size() == 0, "Clear empties the binder");
    }

    // Test 12: Copy-on-write
    {
        cxx::binder<std::string, std::string> b1;
        b1.insert_front("A", "Note A");

        cxx::binder<std::string, std::string> b2 = b1; // Kopia współdzielona
        b2.insert_front("B", "Note B");                // Powoduje rozdzielenie

        assert_true(b1.size() == 1, "Original binder remains unaffected after copy-on-write");
        assert_true(b2.size() == 2, "Modified binder has correct size");
    }
}

void copy_tests() {

    // Test 1:  
    {
        cxx::binder<std::string, std::string> b1;
        b1.insert_front("A", "Note A");

        cxx::binder<std::string, std::string> b2 = b1;
        b2.insert_front("B", "Note B");   

        b2.remove("A");

        assert_true(b1.size() == 1, "Original binder remains unaffected after copy-on-write");
        assert_true(b2.size() == 1, "Modified binder has correct size");
        assert_true(b2.read("B") == "Note B", "Modified binder has correct value");

        try {
            b2.read("A");
            assert_true(false, "Read non-existent key throws exception");
        } catch (const std::invalid_argument&) {
            assert_true(true, "Read non-existent key throws exception");
        }
    }

}

void noexcept_tests() {
    // Sprawdzanie czy metoda `clear` jest noexcept
    assert_true(noexcept(std::declval<cxx::binder<std::string, std::string>>().clear()), 
                "Method clear is noexcept");

    // Sprawdzanie czy konstruktor przenoszący jest noexcept
    assert_true(noexcept(cxx::binder<std::string, std::string>(std::declval<cxx::binder<std::string, std::string>&&>())), 
                "Move constructor is noexcept");

    // Sprawdzanie czy destruktor jest noexcept
    assert_true(noexcept(std::declval<cxx::binder<std::string, std::string>>().~binder()), 
                "Destructor is noexcept");
}

void strong_guarantee_tests() {


    struct ThrowingValue {
        int x;
        ThrowingValue() { throw std::runtime_error("Simulated failure"); }
        ThrowingValue(int x) : x(x) {}

        ThrowingValue(const ThrowingValue& other) : x(other.x) {}

        ThrowingValue(ThrowingValue&& other) {
            x = std::move(other.x);
        }

        bool operator==(const ThrowingValue& other) const {
            return x == other.x;
        }

        bool operator!=(const ThrowingValue& other) const {
            return !(*this == other);
        }

        ThrowingValue& operator=(const ThrowingValue& other) {
            x = other.x;
            return *this;
        }

        ThrowingValue& operator=(ThrowingValue&& other) {
            x = std::move(other.x);
            return *this;
        }
    };

    // Test 1: Insert front
    {
        cxx::binder<std::string, ThrowingValue> binder;
        binder.insert_front("key1", ThrowingValue(1));
        binder.insert_front("key2", ThrowingValue(2));

        auto backup = binder;

        try {
            binder.insert_front("key1", ThrowingValue());
        } catch (const std::invalid_argument &) {
            // ignore
        } catch (...) {
            // ignore
        }

        bool g1 = binder.size() == backup.size();
        bool g2 = binder.read("key1") == backup.read("key1");
        bool g3 = binder.read("key2") == backup.read("key2");

        assert_true(g1 && g2 && g3, "Insert front strong guarantee");
    }

    // Test 2: Insert after
    {
        cxx::binder<std::string, ThrowingValue> binder;
        binder.insert_front("key1", ThrowingValue(1));
        binder.insert_front("key2", ThrowingValue(2));

        auto backup = binder;

        try {
            binder.insert_after("key2", "key3", ThrowingValue());
        } catch (const std::invalid_argument &) {
            // ignore
        } catch (...) {
            // ignore
        }

        bool g1 = binder.size() == backup.size();
        bool g2 = binder.read("key1") == backup.read("key1");
        bool g3 = binder.read("key2") == backup.read("key2");

        assert_true(g1 && g2 && g3, "Insert after strong guarantee");
    }

    // Test 3: remove
    {
        cxx::binder<std::string, ThrowingValue> binder;

        auto backup = binder;
        backup.insert_front("key1", ThrowingValue(1));
        backup.insert_front("key2", ThrowingValue(2));

        try {
            binder.remove();
        } catch (const std::invalid_argument &) {
            // ignore
        } catch (...) {
            // ignore
        }

        binder.insert_front("key1", ThrowingValue(1));
        binder.insert_front("key2", ThrowingValue(2));

        bool g1 = binder.size() == backup.size();
        bool g2 = binder.read("key1") == backup.read("key1");
        bool g3 = binder.read("key2") == backup.read("key2");

        assert_true(g1 && g2 && g3, "Remove strong guarantee");
    }

    // Test 4: non-const read
    {
        cxx::binder<std::string, ThrowingValue> binder;
        binder.insert_front("key1", ThrowingValue(1));
        binder.insert_front("key2", ThrowingValue(2));

        ThrowingValue& a = binder.read("key1");

        a = ThrowingValue(3);
        assert_true(a == ThrowingValue(3), "Non-const read allows modification");

        auto backup = binder;

        try {
            a = binder.read("key3");
        } catch (const std::invalid_argument &) {
            // ignore
        } catch (...) {
            // ignore
        }

        bool g1 = binder.size() == backup.size();
        bool g2 = binder.read("key1") == backup.read("key1");
        bool g3 = binder.read("key2") == backup.read("key2");

        assert_true(g1 && g2 && g3, "Non-const read strong guarantee");
    }

    // Test 5: const read
    {
        cxx::binder<std::string, ThrowingValue> binder;
        binder.insert_front("key1", ThrowingValue(1));
        binder.insert_front("key2", ThrowingValue(2));

        auto backup = binder;

        try {
            const ThrowingValue& a = binder.read("key3"); // can be warned by compiler
        } catch (const std::invalid_argument &) {
            // ignore
        } catch (...) {
            // ignore
        }
        
        bool g1 = binder.size() == backup.size();
        bool g2 = binder.read("key1") == backup.read("key1");
        bool g3 = binder.read("key2") == backup.read("key2");

        assert_true(g1 && g2 && g3, "Const read strong guarantee");
    }

}

int main() {
    simple_tests();
    gpt_tests();
    copy_tests();
    noexcept_tests();
    strong_guarantee_tests();
    return 0;
}

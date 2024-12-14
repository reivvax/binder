#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>

namespace cxx {
    template <typename K, typename V>
    class binder {
        std::list<V> data;
        std::map<K, typename std::list<V>::iterator> iters;

    public:
        constexpr binder() = default;

        template <typename T, typename U>
        constexpr binder(const binder<T, U>& rhs);

        template <typename T, typename U>
        constexpr binder(binder<T, U>&& rhs);


        constexpr binder& operator=(binder rhs);

        constexpr void insert_front(K const& k, V const& v);

        constexpr void insert_afer(K const& prev_k, K const& k, V const& v);

        constexpr void remove();

        constexpr void remove(K const& k);

        constexpr V& read(K const& k);
        constexpr V const& read(K const& k) const;

        constexpr size_t size() const noexcept {
            return data.size(); // noexcept
        }

        constexpr void clear() noexcept {
            data.clear();
            iters.clear();
        }


        class const_iterator {
            typename std::list<V>::const_iterator current;

        public:
            using value_type = V;

            explicit const_iterator(typename std::list<V>::const_iterator it)
                : current(it) {}

            const V& operator*() const { return *current; }
            const V* operator->() const { return &(*current); }

            const_iterator& operator++() {
                ++current;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const const_iterator& other) const {
                return current == other.current;
            }

            bool operator!=(const const_iterator& other) const {
                return !(*this == other);
            }
        };

        const_iterator cbegin() const noexcept { return const_iterator(data.cbegin()); }
        const_iterator cend() const noexcept { return const_iterator(data.cend()); }



    };
}

#endif //BINDER_H

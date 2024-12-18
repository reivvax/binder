#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>
#include <memory>

namespace cxx {
    template <typename K, typename V>
    class binder {
        struct Data {
            std::list<V> data;
            std::map<K, typename std::list<V>::iterator> iters;
        };

        std::shared_ptr<Data> data_ptr;
        
        void ensure_unique() { // except
            if (!data_ptr.unique()) {
                data_ptr = std::make_shared<Data>(*data_ptr);
            }
        }

    public:
        binder() : data_ptr(std::make_shared<Data>()) {} // except

        // co jeżeli typy się nie zgadzają
        template <typename T, typename U>
        binder(const binder<T, U>& rhs) noexcept : data_ptr(rhs.data_ptr) {}

        template <typename T, typename U>
        binder(binder<T, U>&& rhs) noexcept : data_ptr(std::move(rhs.data_ptr)) {
            rhs.data_ptr = nullptr;
        }

        constexpr binder& operator=(binder rhs);

        void insert_front(K const& k, V const& v) { // except
            ensure_unique();
            
            if (data_ptr->iters.find(k) != data_ptr->iters.end()) {
                throw std::invalid_argument("Key already exists");
            }

            std::list<V> temp_list = data_ptr->data;
            auto temp_iters = data_ptr->iters;

            temp_list.push_front(v);
            temp_iters[k] = temp_list.begin();

            std::swap(data_ptr->data, temp_list);
            std::swap(data_ptr->iters, temp_iters);
        }

        constexpr void insert_afer(K const& prev_k, K const& k, V const& v);

        constexpr void remove();

        constexpr void remove(K const& k);

        constexpr V& read(K const& k);
        constexpr V const& read(K const& k) const;

        constexpr size_t size() const noexcept {
            return data_ptr->data.size(); // noexcept
        }

        constexpr void clear() noexcept {
            data_ptr->data.clear();
            data_ptr->iters.clear();
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

        const_iterator cbegin() const noexcept { return const_iterator(data_ptr->data.cbegin()); }
        const_iterator cend() const noexcept { return const_iterator(data_ptr->data.cend()); }



    };
}

#endif //BINDER_H

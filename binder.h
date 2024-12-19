#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>
#include <memory>

namespace cxx {
    template <typename K, typename V>
    class binder {
        struct Data {
            std::list<std::pair<K, V>> data;
            std::map<K, typename std::list<std::pair<K, V>>::iterator> iters;
        };

        std::shared_ptr<Data> data_ptr;
        
        void ensure_unique() {
            if (!data_ptr.unique()) {
                std::shared_ptr<Data> new_data_ptr = std::make_shared<Data>();

                for (const auto& item : data_ptr->data) {
                    new_data_ptr->data.push_back({item.first, item.second});
                    auto it = std::prev(new_data_ptr->data.end());
                    new_data_ptr->iters[item.first] = it;
                }

                data_ptr = new_data_ptr;
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

        // tymczasowo skomentowano
        // constexpr binder& operator=(binder rhs);

        void insert_front(K const& k, V const& v) {
            ensure_unique();
            
            if (data_ptr->iters.find(k) != data_ptr->iters.end()) {
                throw std::invalid_argument("Key already exists");
            }

            data_ptr->data.push_front({k, v});
            data_ptr->iters[k] = data_ptr->data.begin();
        }

        constexpr void insert_afer(K const& prev_k, K const& k, V const& v);

        void remove() { // except
            ensure_unique();

            if (size() == 0) {
                throw std::invalid_argument("Binder is empty");
            }

            K k = data_ptr->data.front().first;
            auto it = data_ptr->iters.find(k);

            data_ptr->iters.erase(it);
            data_ptr->data.pop_front();
        }

        constexpr void remove(K const& k);

        constexpr V& read(K const& k) { // except
            ensure_unique();
            auto it = data_ptr->iters.find(k);
            if (it == data_ptr->iters.end()) {
                throw std::invalid_argument("Key does not exist");
            }
            return it->second->second;
        }

        constexpr V const& read(K const& k) const {
            auto it = data_ptr->iters.find(k);
            if (it == data_ptr->iters.end()) {
                throw std::invalid_argument("Key does not exist");
            }
            return it->second->second;
        }

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

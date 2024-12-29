#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>
#include <memory>
#include <iterator>

#include <iostream>

namespace cxx {
    template <typename K, typename V>
    class binder {
        using data_list = std::list<std::pair<K, V>>;
        using data_iter = typename data_list::iterator;
        using iters_map = std::map<K, typename data_list::iterator>;

        struct Data {
            data_list data;
            iters_map iters;
        };

        std::shared_ptr<Data> data_ptr;
        bool was_mutable_read;

        std::shared_ptr<Data> ensure_unique() {
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }
            else if (!data_ptr.unique()) {
                std::shared_ptr<Data> new_data_ptr = std::make_shared<Data>();

                for (const auto& item : data_ptr->data) {
                    new_data_ptr->data.push_back({item.first, item.second});
                    auto it = std::prev(new_data_ptr->data.end());
                    new_data_ptr->iters[item.first] = it;
                }

                std::shared_ptr<Data> res = move(data_ptr);
                data_ptr = new_data_ptr;

                return res; // move?
            }
            return data_ptr;
        }

    public:
        binder() : data_ptr(std::make_shared<Data>()), was_mutable_read(false) {} // except

        binder(const binder<K, V>& rhs) : data_ptr(rhs.data_ptr) {
            if (rhs.was_mutable_read)
                ensure_unique();
        }

        binder(binder<K, V>&& rhs) noexcept : data_ptr(std::move(rhs.data_ptr)), was_mutable_read(false) {
            rhs.data_ptr = nullptr;
        }
        
        ~binder() = default;

        binder& operator=(binder<K, V> rhs) {
            data_ptr = std::move(rhs.data_ptr);
            was_mutable_read = false;
        }

        void insert_front(K const& k, V const& v) { // except
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }

            if (data_ptr->iters.find(k) != data_ptr->iters.end()) {
                throw std::invalid_argument("Key already exists");
            }

            std::shared_ptr<Data> prev = ensure_unique();

            try {
                data_ptr->data.push_front({k, v});          // strong gurantee
            } catch (...) {
                data_ptr = move(prev);
                throw;
            }

            try {
                auto it = data_ptr->data.begin();       // no-throw gurantee
                data_ptr->iters[k] = it;                // strong_gurantee
                was_mutable_read = false;
            } catch (...) {
                data_ptr->data.pop_front();             // no-throw gurantee
                data_ptr = move(prev);
                throw;
            }
        }

        void insert_after(K const& prev_k, K const& k, V const& v) { // except
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }

            auto map_iter = data_ptr->iters.find(prev_k);
            
            if (data_ptr->iters.find(k) != data_ptr->iters.end() 
                || map_iter == data_ptr->iters.end()) {
                    throw std::invalid_argument("Key already exists");
            }

            ensure_unique();
            
            auto position = map_iter->second;

            ++position;                                 // iterator points to the element after prev_k
            data_ptr->data.insert(position, {k, v});    // strong guarantee
            --position;                                 // iterator points to inserted element
            
            try {
                data_ptr->iters[k] = position;          // strong guarantee
                was_mutable_read = false;
            } catch (...) {
                data_ptr->data.erase(position);         // no-throw
                throw;
            }
        }

        void remove() { // except
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }

            if (size() == 0) {
                throw std::invalid_argument("Binder is empty");
            }

            K k = data_ptr->data.front().first;
            auto it = data_ptr->iters.find(k);          // strong gurantee

            ensure_unique();

            data_ptr->iters.erase(it);                  // no-throw gurantee / strong guarantee

            data_ptr->data.pop_front();                 // no-throw guarantee
            was_mutable_read = false;
        }

        constexpr void remove(K const& k) { // except
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }

            auto map_iter = data_ptr->iters.find(k);

            if (map_iter == data_ptr->iters.end()) {
                throw std::invalid_argument("Binder does not contain specified key");
            }

            ensure_unique();

            auto position = map_iter->second;

            data_ptr->iters.erase(map_iter);            // no-throw
            data_ptr->data.erase(position);             // no-throw
            was_mutable_read = false;
        }

        constexpr V& read(K const& k) { // except
            if (!data_ptr) {
                data_ptr = std::make_shared<Data>();
            }

            auto it = data_ptr->iters.find(k);
            if (it == data_ptr->iters.end()) {
                throw std::invalid_argument("Key does not exist");
            }

            was_mutable_read = true;

            ensure_unique();
            return it->second->second;
        }

        constexpr V const& read(K const& k) const {
            if (!data_ptr) {
                throw std::invalid_argument("Key does not exist");
            }

            auto it = data_ptr->iters.find(k);
            if (it == data_ptr->iters.end()) {
                throw std::invalid_argument("Key does not exist");
            }
            return it->second->second;
        }

        constexpr size_t size() const noexcept {
            if (!data_ptr) {
                return 0;
            }
            return data_ptr->data.size();
        }

        constexpr void clear() noexcept {
            was_mutable_read = false;
            if (data_ptr && data_ptr.unique()) {
                data_ptr->data.clear();
                data_ptr->iters.clear();
            } else {
                // TODO: should make_shared, but it throws exceptions
                data_ptr = nullptr;
            }
        }

        class const_iterator {
            typename data_list::const_iterator current;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = V;
            using pointer = const V*;
            using reference = const V&;
            using itarator_category = std::forward_iterator_tag;

            const_iterator() = default;

            const_iterator(const const_iterator& rhs) : current(rhs.current) {}

            explicit const_iterator(typename data_list::const_iterator it)
                : current(it) {}

            ~const_iterator() = default;

            const V& operator*() const noexcept { return (*current).second; }
            const V* operator->() const noexcept { return &((*current).second); }

            const_iterator& operator++() noexcept {
                ++current;
                return *this;
            }

            const_iterator operator++(int) noexcept {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            const_iterator& operator=(const_iterator const& rhs) {
                current = rhs.current;
            }

            bool operator==(const const_iterator& other) const noexcept {
                return current == other.current; // no-except https://en.cppreference.com/w/cpp/iterator/basic_const_iterator
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

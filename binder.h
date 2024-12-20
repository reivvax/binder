#ifndef BINDER_H
#define BINDER_H

#include <list>
#include <map>
#include <memory>

namespace detail {
    template<typename K, typename V, typename K2, typename V2>
    concept convertible_binder = std::convertible_to<K2, K> && std::convertible_to<V2, V>;
}

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

        // TODO jeżeli się typy nie zgadzają, to chyba nie musi się kompilować
        binder(const binder<K, V>& rhs) noexcept : data_ptr(rhs.data_ptr) {}

        binder(binder<K, V>&& rhs) noexcept : data_ptr(std::move(rhs.data_ptr)) {
            rhs.data_ptr = nullptr;
        }

        template <typename K2, typename V2>
        requires detail::convertible_binder<K, V, K2, V2>
        binder& operator=(binder<K2, V2> rhs) {
            data_ptr = rhs.data_ptr;
        }

        void insert_front(K const& k, V const& v) {
            ensure_unique();
            
            if (data_ptr->iters.find(k) != data_ptr->iters.end()) {
                throw std::invalid_argument("Key already exists");
            }

            data_ptr->data.push_front({k, v}); // strong gurantee

            try {
                auto it = data_ptr->data.begin(); // no-throw gurantee
                data_ptr->iters[k] = it; // strong_gurantee
            } catch (...) {
                data_ptr->data.pop_front(); // no-throw gurantee
                throw;
            }
        }

        void insert_afer(K const& prev_k, K const& k, V const& v) {
            auto position = data_ptr->iters.find(prev_k);

            if (data_ptr->iters.find(k) != data_ptr->iters.end() 
                || position == data_ptr->iters.end()) {
                    throw std::invalid_argument("Key already exists");
            }

            ensure_unique();
            
            position++; // iterator points to the element after the new element
            data_ptr->data.insert(position, {k, v}); // strong guarantee
            position--; // iterator points to inserted element
            
            try {
                data_ptr->iters[k] = position; // strong guarantee
            } catch (...) {
                data_ptr->data.erase(position);
                throw;
            }
        }

        void remove() { // except
            ensure_unique();

            if (size() == 0) {
                throw std::invalid_argument("Binder is empty");
            }

            K k = data_ptr->data.front().first;
            data_ptr->data.pop_front(); // no-throw gurantee
            auto it = data_ptr->iters.find(k); // strong gurantee
            data_ptr->iters.erase(it); // no-throw gurantee / strong gurantee
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

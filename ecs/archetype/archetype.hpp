#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <any>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>

#include "../types.hpp"

namespace gxe {

class archetype {
public:
    archetype() = default; // Empty archetype.
    ~archetype() = default;

    struct signature { // SIGNATURE.
    public:
        bool operator==(const signature& other) const {
            return _components == other._components;
        }

        bool contains(const component_id id) const {
            return _components.find(id) != _components.end();
        }

        bool contains_all(const signature& other) const {
            for (component_id id : other._components) {
                if (!contains(id)) return false;
            }

            return true;
        }

        void insert(component_id id) {
            _components.insert(id);
        }

        void remove(component_id id) {
            _components.erase(id);
        }

        const std::unordered_set<component_id>& data() const {
            return _components;
        }

        size_t size() const {
            return _components.size();
        }

        const std::unordered_set<component_id>& components() const {
            return _components;
        }

        // Hash impl for signature
        // TODO: Rework this to either use an inline bitset or a vector<bool>.
        struct hash {
            size_t operator()(const signature& sig) const {
                size_t hash_value = 0;
                for (component_id id : sig._components) {
                    uint64_t x = id + 0x9e3779b97f4a7c15ull;            // splitmix64 finalizer
                    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
                    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
                    hash_value += static_cast<size_t>(x ^ (x >> 31));   // + doesn't care about order
                }
                return hash_value;
            }
        };

    private:
        std::unordered_set<component_id> _components;
    }; // SIGNATURE

    // TODO: Manual mem offset without type concerns.
    struct component_array_base {
        virtual ~component_array_base() = default;

        virtual void* get(size_t index) = 0;
        virtual const void* get(size_t index) const = 0;

        virtual void push_back(std::any&& component) = 0;
        virtual void remove(size_t index) = 0;

        virtual size_t size() const = 0;
        virtual void reserve(size_t capacity) = 0;
    };

    template<typename C>
    struct component_array : component_array_base {
        std::vector<C> _data;

        void* get(size_t index) override {
            return &_data[index];
        }
        
        const void* get(size_t index) const override {
            return &_data[index];
        }
        
        void push_back(std::any&& component) override {
            _data.push_back(std::move(std::any_cast<C&>(component)));
        }
        
        // Swap and pop.
        // This needs to somehow enable updating
        // of the swapped entity record.
        void remove(size_t index) override {
            if (index < _data.size() - 1) {
                std::swap(_data[index], _data.back());
            }
            _data.pop_back();
        }
        
        size_t size() const override {
            return _data.size();
        }
        
        void reserve(size_t capacity) override {
            _data.reserve(capacity);
        }
    };
    // End component arrays

    void insert_component_array(const component_id id, std::unique_ptr<component_array_base> array){
        size_t index = _data.size();
        _data.push_back(std::move(array));
        _componentMap[id] = index;
        _signature.insert(id);
    }

    void insert_component(component_id id, std::any&& component) {
        auto it = _componentMap.find(id);
        assert(it != _componentMap.end() && "Attempted to insert component in incompatible archetype");
        
        size_t array_index = it->second;
        component_array_base* arr = _data[array_index].get();

        arr->push_back(std::move(component));
    }
    
    // TODO: Use flat struct
    std::optional<size_t> delete_entity_components(const size_t arch_index){
        const size_t rows = size(); // size() guards the component-less archetype case.
        if(rows == 0) return std::nullopt;

        const size_t last_index = rows - 1;

        for (auto& dat : _data) {
            assert(arch_index < dat->size());
            dat->remove(arch_index);
        }

        if(arch_index == last_index){
            return std::nullopt; // No update needed, we removed the last element.
        }

        return last_index;
    }

    const signature& getSignature() const {
        return _signature;
    }

    // Number of entities stored in this archetype (all component arrays are kept in lockstep).
    size_t size() const {
        return _data.empty() ? 0 : _data.front()->size();
    }

    // Typed access to a whole column. There is deliberately no component_id parameter:
    // C alone determines the column, so a type/id mismatch is unrepresentable.
    // 
    // I love you Aidan <3 / I LOVE YOU TOO LUBBINGON BIGAMUS
    template<typename C>
    component_array<C>& array_for() {
        auto it = _componentMap.find(get_component_id<C>());
        assert(it != _componentMap.end() && "Component not present in this archetype");
        return *static_cast<component_array<C>*>(_data[it->second].get());
    }

    template<typename C>
    std::span<C> column() {
        return std::span<C>(array_for<C>()._data);
    }

    template<typename C>
    void push_component(C&& value) {
        array_for<std::remove_cvref_t<C>>()._data.push_back(std::forward<C>(value));
    }

private:
    std::unordered_map<component_id, size_t> _componentMap; // Map from component id to their index in this specific archetype collection
    std::vector<std::unique_ptr<component_array_base>> _data;
    signature _signature;
};

}; // namespace gxe
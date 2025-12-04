#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <any>
#include <memory>

#include "../types.hpp"

namespace gxe {

// An archetype is a collection of components, that are related in that each of the
// entities that own one component in the archetype own one of every
// component in the archetype.

// Each archetype maintains a vector for each of its underlying components.

// Each archetype has a signature. Entities themselves to not maintain a signature,
// any time an entity needs to use a signature, it should be provided by the archetype.

class archetype {
public:
    archetype() = default; // Empty archetype.
    ~archetype() = default;

    // Wrapper around a dynamic bitset (currentl vector<bool>)
    struct signature { // SIGNATURE.
    public:
        // Equality operator (not assignment operator)
        bool operator==(const signature& other) const {
            return _components == other._components;
        }

        // Check if the archetype contains the component ID.
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

        // Expose data for hashing
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
        struct hash {
            size_t operator()(const signature& sig) const {
                size_t hash_value = 0;
                for (component_id id : sig._components) {
                    hash_value ^= std::hash<component_id>{}(id) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
                }
                return hash_value;
            }
        };

    private:
        std::unordered_set<component_id> _components;
    }; // SIGNATURE

    // Component arrays
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
        
        // Swap and pop
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
        // Move a component array into our component map and _data
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

        // Automatically handles the type cast via push back
        arr->push_back(std::move(component));
    }
    
    // Remove the components for entity with at arch_idx (swap pop, update entity record.)
    void delete_entity_components(const size_t arch_idx){
        
    }

    const signature& getSignature() const {
        return _signature;
    }

private:
    std::unordered_map<component_id, size_t> _componentMap; // Map from componentid to their index in this specific archetype collection
    std::vector<std::unique_ptr<component_array_base>> _data;
    signature _signature;
};

}; // namespace gxe
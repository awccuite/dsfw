#pragma once

#include <unordered_map>
#include <vector>
#include <iostream>
#include <any>

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
    struct signature { // SIGNATURE
    public:
        // Equality operator (not assignment operator)
        bool operator==(const signature& other) const {
            return _data == other._data;
        }

        // Check if the archetype contains the component ID.
        bool contains(const component_id id) const {
            [[likely]] try {
                return _data.at(static_cast<size_t>(id));
            } catch (...) {
                std::cout << "Tried to check unregistered component existence of ID: " << id << std::endl; 
                throw;
            }

            return false;
        }

        void insert(component_id id){
            _data[(static_cast<size_t>(id))] = true;
        }

        // Expose data for hashing
        const std::vector<bool>& data() const {
            return _data;
        }

        // Hash impl for signature
        struct hash {
            size_t operator()(const signature& sig) const {
                const auto& data = sig.data();
                size_t hash_value = 0;
                
                for (size_t i = 0; i < data.size(); ++i) {
                    if (data[i]) {
                        hash_value ^= std::hash<size_t>{}(i) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
                    }
                }
                
                return hash_value;
            }
        };

    private:
        std::vector<bool> _data;
    }; // SIGNATURE

private:
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
        
        void push_back(std::any&& component) override { // any_cast is effecitvely a static_cast of the object.
            _data.push_back(std::any_cast<C>(std::move(component)));
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

    signature& getSignature() {
        return _signature;
    }

    std::unordered_map<component_id, size_t> _componentMap; // Map from componentid to their index in this specific archetype collection
    std::vector<std::unique_ptr<component_array_base>> _data;
    signature _signature;
};

}; // namespace gxe
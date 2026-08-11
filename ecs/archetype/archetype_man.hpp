#pragma once

#include "archetype.hpp"

#include <memory>
#include <atomic>
#include <string>
#include <functional>
#include <iostream>
#include <tuple>
#include <utility>
#include <type_traits>

namespace gxe {

class archetype_manager {
using component_factory = std::function<std::unique_ptr<archetype::component_array_base>()>;

public:
    // Forwards to gxe::get_component_id (types.hpp). The counter lives there so that
    // archetype itself can resolve type -> id without a circular include.
    template<typename Component>
    static component_id get_component_id(){
        return gxe::get_component_id<Component>();
    }

    // Signature for a component pack. Cached in a function-local static, so this is
    // allocation-free after the first call - it used to return by value, copying an
    // unordered_set on every query and every spawn.
    template<typename ...Components>
    static const archetype::signature& signature_for(){
        static const archetype::signature sig = [](){
            archetype::signature s;
            (s.insert(gxe::get_component_id<Components>()), ...);
            return s;
        }();
        return sig;
    }

    template<typename C>
    uint32_t register_component(){
        component_id id = get_component_id<C>();
        _componentNames[id] = typeid(C).name();
        
        // Static per C method for creating a component array for said method
        _componentFactories[id] = []() -> std::unique_ptr<archetype::component_array_base> {
            return std::make_unique<archetype::component_array<C>>();
        };

        return id;
    }

    // Get or create an archetype based on the requested signature, returning its index.
    // Indices are stable: _archetypes is append-only and never reorders.
    size_t get_or_create_archetype_index(const archetype::signature& signature){
        auto it = _archetypeMap.find(signature);
        if(it != _archetypeMap.end()){
            return it->second;
        }

        return create_archetype(signature);
    }

    archetype& archetype_at(const size_t index){
        return *(_archetypes[index]._data);
    }

    // Appends one row across every column of arch.
    template<typename ...Cs>
    void append_row(archetype& arch, Cs&&... comps){
        (arch.push_component(std::forward<Cs>(comps)), ...);
    }

    // Invokes f(Components&...) for every entity owning all of Components.
    // Column pointers are resolved once per archetype, not per row.
    template<typename ...Components, typename F>
    void for_each_with_components(F&& f){
        const archetype::signature& sig = signature_for<std::remove_cvref_t<Components>...>();

        const std::vector<archetype*>& matches = matching_archetypes(sig);

        for(archetype* archp : matches){
            archetype& arch = *archp;
            const size_t count = arch.size();
            if(count == 0) continue;

            // One hash lookup + one devirtualized resolve per component, per archetype.
            std::tuple<std::remove_cvref_t<Components>*...> cols{
                arch.template column<std::remove_cvref_t<Components>>().data()...
            };

            // Index-based expansion rather than std::get<T>, so a repeated component type
            // in the pack still compiles (both pointers simply alias the same column).
            [&]<size_t ...I>(std::index_sequence<I...>){
                for(size_t i = 0; i < count; ++i){
                    f(std::get<I>(cols)[i]...);
                }
            }(std::index_sequence_for<Components...>{});
        }
    }

private:
    // Archetypes matching a query signature.
    //
    // Never needs invalidating, only extending: _archetypes is append-only and never reorders,
    // and each archetype is separately heap-allocated behind unique_ptr, so archetype* stays
    // valid across vector reallocation. _scanned records how much of _archetypes this entry has
    // already been tested against, so a later call only tests the new tail.
    //
    // NOTE: if archetype *removal* is ever added, this cache must be cleared.
    struct query_cache_entry {
        std::vector<archetype*> _matches;
        size_t _scanned = 0;
    };

    const std::vector<archetype*>& matching_archetypes(const archetype::signature& sig){
        query_cache_entry& entry = _queryCache[sig];

        for(size_t i = entry._scanned; i < _archetypes.size(); ++i){
            if(_archetypes[i]._signature.contains_all(sig)){
                entry._matches.push_back(_archetypes[i]._data.get());
            }
        }
        entry._scanned = _archetypes.size();

        return entry._matches;
    }

    size_t create_archetype(const archetype::signature& signature){
        archetype_wrapper wrapper;
        wrapper._signature = signature; // Copy construct the signature
        wrapper._data = std::make_unique<archetype>();

        for(const component_id& id : signature.components()){
            auto it = _componentFactories.find(id);
            if(it == _componentFactories.end()){
                throw std::runtime_error("Component not registered: " + std::to_string(id));
            }
            wrapper._data->insert_component_array(id, it->second());
        }

        const size_t index = _archetypes.size();
        _archetypes.push_back(std::move(wrapper));
        _archetypeMap[signature] = index;

        return index;
    }

    struct archetype_wrapper {
        archetype::signature _signature;
        std::unique_ptr<archetype> _data; 
    };

    // We intentionally store duplicates of the signatures, as the map provides O(1) specific archetype lookup, and the vector provides efficient iteration over signatures.
    std::vector<archetype_wrapper> _archetypes; // Archetype container. Useful for queries like "forEachWith<Components...>()". We can always cache queries as well
    std::unordered_map<archetype::signature, size_t, archetype::signature::hash> _archetypeMap; // Map each archetype signature to its index in _archetypes.
    std::unordered_map<component_id, component_factory> _componentFactories;

    // Per-instance (not static): separate worlds must not share cached archetype pointers.
    std::unordered_map<archetype::signature, query_cache_entry, archetype::signature::hash> _queryCache;

    static inline std::unordered_map<component_id, std::string> _componentNames;
};

}; // namespace gxe
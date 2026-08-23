#pragma once

#include "types.hpp"

#include <vector>

namespace gxe {

class id_manager {
public:
    id_manager() { allocate_entities(k_initial_entity_id); }
    ~id_manager() = default;

    // Return an id, and remove it from availableEntities
    entity_id create_entity() {
        entity_id id = _availableIds.back();
        _availableIds.pop_back();

        if(_availableIds.empty()){
            // Refill starting AFTER the id just handed out - starting at `id` would reissue it,
            // producing two live entities sharing one id every k_initial_entity_limit spawns.
            allocate_entities(id + 1);
        }

        _numEntities++;
        return id;
    }

    void destroy_entity(entity_id id) {
        _availableIds.push_back(id);
        _numEntities--;
    }

    int entity_count() const { return _numEntities; };

private:
    static constexpr uint32_t  k_initial_entity_limit = 1024;
    static constexpr entity_id k_initial_entity_id    = 0;

    void allocate_entities(entity_id startId) {
        entity_id endId = startId + k_initial_entity_limit;
        _availableIds.reserve(_availableIds.size() + k_initial_entity_limit);

        for(entity_id i = endId - 1; i >= startId; i--){
            _availableIds.emplace_back(i);
            [[unlikely]] if(i == 0){
                break;
            }
        }
    }

    std::vector<entity_id> _availableIds; // Treat as stack for uniqueID's.

    uint32_t _numEntities{0};
};

}

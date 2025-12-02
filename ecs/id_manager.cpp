#include <cstdint>

#include "id_manager.hpp"

namespace gxe {

constexpr uint32_t INITIAL_ENTITY_LIMIT = 1024;
const entity_id INITIAL_ENTITY_ID = 0;

id_manager::id_manager() : _numEntities(0) {
    allocate_entities(INITIAL_ENTITY_ID);
}

entity_id id_manager::create_entity(){
    // Pop the back of available.
    entity_id id = _availableIds.back();
    _availableIds.pop_back();

    if(_availableIds.empty()){
        allocate_entities(id);
    }

    _numEntities++;
    return id;
}

// We want Id's to be reused in the potential
// case that we delete and spawn entities in vast quantities.
void id_manager::destroy_entity(entity_id id){
    _availableIds.push_back(id);
    _numEntities--;
}

void id_manager::allocate_entities(entity_id startId){
    entity_id endId = startId + INITIAL_ENTITY_LIMIT;
    _availableIds.reserve(_availableIds.size() + INITIAL_ENTITY_LIMIT);

    for(entity_id i = endId - 1; i >= startId; i--){
        _availableIds.emplace_back(i);
        [[unlikely]] if(i == 0){
            break;
        }
    }
}

} // namespace gxe
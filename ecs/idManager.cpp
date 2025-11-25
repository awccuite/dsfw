#include <cstdint>

#include "idManager.hpp"

namespace gxe {

const uint32_t INITIAL_ENTITY_LIMIT = INITIAL_SPARSE_SET_CAPACITY; // Set an initial entity limit of 1024 entities.
const entityid INITIIAL_ENTITY_ID = 0;

idManager::idManager() : _numEntities(0) {
    allocateEntities(INITIIAL_ENTITY_ID);
}

entityid idManager::createEntity(){
    // Pop the back of available.
    entityid id = _availableIds.back();
    _availableIds.pop_back();

    if(_availableIds.empty()){
        allocateEntities(id);
    }

    _numEntities++;
    return id;
}

// We want Id's to be reused in the potential
// case that we delete and spawn entities in vast quantities.
void idManager::destroyEntity(entityid id){
    _availableIds.push_back(id);
    _numEntities--;
}

void idManager::allocateEntities(entityid startId){
    entityid endId = startId + INITIAL_ENTITY_LIMIT;
    _availableIds.reserve(_availableIds.size() + INITIAL_ENTITY_LIMIT);

    for(entityid i = endId; i > startId; i--){
        _availableIds.emplace_back(i);
    }
}

} // namespace gxe
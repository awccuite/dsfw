#pragma once

#include "types.hpp"

#include <vector>

namespace gxe {

class id_manager {
public:
    id_manager();
    ~id_manager() = default;

    entity_id create_entity(); // Return an id, and remove it from availableEntities
    void destroy_entity(entity_id id);

    int entity_count() const { return _numEntities; };

private:
    void allocate_entities(entity_id startID);
    std::vector<entity_id> _availableIds; // Treat as stack for uniqueID's.

    uint32_t _numEntities;
};

}
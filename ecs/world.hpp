#pragma once

#include "system.hpp"
#include "idManager.hpp"
#include "archetype/archetype_man.hpp"

namespace gxe {

class World {
public:


private:
    idManager _idManager;
    ArchetypeManager _archetypes;

    std::vector<std::unique_ptr<SystemBase>> _systems;

    bool _initialized = false;
};

}; // namespace gxe
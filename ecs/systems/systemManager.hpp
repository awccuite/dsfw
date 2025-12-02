#pragma once

#include "system.hpp"

namespace gxe {

class systemManager {
public:

private:
    std::vector<std::unique_ptr<system_base>> _systems;

};

}; // namespace gxe
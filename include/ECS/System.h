#ifndef SYSTEM_H
#define SYSTEM_H

#include <set>
#include "Entity.h"

class System {
public:
    std::set<Entity> m_Entities;
};

#endif // SYSTEM_H

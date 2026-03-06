#pragma once
#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>
#include <limits>

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 100000;
const Entity NULL_ENTITY = std::numeric_limits<Entity>::max();

#endif // ENTITY_H

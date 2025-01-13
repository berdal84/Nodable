#pragma once
#include <concepts>
#include "tools/gui/geometry/SpatialNode.h"

namespace ndbl
{
    template<typename T>
    concept CView = requires(T *t) {
        { t->spatial_node() } -> std::same_as<tools::SpatialNode *>;
    };
}

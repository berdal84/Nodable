#pragma once

#include "fw/core/types.h"
#include <fw/core/reflection/reflection>

#include <ndbl/core/Component.h>

namespace ndbl
{
    /**
     * @deprecated Component to handle node read/write from/to disk
     */
	class DataAccess : public Component
	{
	public:
		DataAccess() {};
		bool update()override;  // Write the node owner of this component to disk (as JSON)

        REFLECT_DERIVED_CLASS(Component)
	};
}
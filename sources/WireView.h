#pragma once

#include "Nodable.h"
#include "View.h" // base class
#include <imgui/imgui.h>

namespace Nodable{
	class WireView : public View
	{
	public:
		bool draw()override;
	};
}
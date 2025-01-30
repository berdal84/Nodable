#include "ViewState.h"
#include "ImGuiEx.h"

#ifdef TOOLS_DEBUG
#define DEBUG_DRAW 1
#endif

using namespace tools;

ViewState::ViewState(Flags flags)
: _flags(flags)
{
}

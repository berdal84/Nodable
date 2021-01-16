#include "Way.h"

using namespace Nodable;

std::string Nodable::WayToString(Way _way)
{
    switch(_way)
    {
        case Way_In:    return "In";
        case Way_Out:   return "Out";
        case Way_InOut: return "InOut";
        default:        return "None";
    }
}
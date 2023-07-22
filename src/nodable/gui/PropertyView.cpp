#include "NodeView.h"
#include "PropertyView.h"
#include "PropertyConnector.h"

using namespace ndbl;
using Side = ndbl::PropertyConnector::Side;

PropertyView::PropertyView(Property * _property, NodeView* _nodeView)
        : m_property(_property)
        , m_showInput(false)
        , m_touched(false)
        , m_in(nullptr)
        , m_out(nullptr)
        , m_nodeView(_nodeView)
{
    FW_ASSERT(_property ); // Property must be defined
    FW_ASSERT(_nodeView ); // Property must be defined
    if (m_property->allows_connection(Way_In) ) m_in  = new PropertyConnector(this, Way_In, Side::Top);
    if (m_property->allows_connection(Way_Out) ) m_out = new PropertyConnector(this, Way_Out, Side::Bottom);
}

PropertyView::~PropertyView()
{
    delete m_in;
    delete m_out;
}

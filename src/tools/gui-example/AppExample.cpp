#include "AppExample.h"
#include "AppExampleView.h"

using namespace tools;

void AppExample::init()
{
    // init a config and a view (this class owns them!)
    m_config = init_config();
    m_view.init(this);

    // Init the base application using our data
    m_base_app.init_ex(m_view.base_view_handle(), m_config );

    //
    // Your code here
    //
}

void AppExample::run()
{
    m_base_app.run();
}

void AppExample::update()
{
    m_base_app.update();
    //
    // Your code here
    //
}

void AppExample::draw()
{
    m_view.draw();
}

void AppExample::shutdown()
{
    // Shutdown our stuff
    m_base_app.shutdown();
    m_view.shutdown();
    shutdown_config(m_config);

    //
    // Your code here
    //
}

void AppExample::request_stop()
{
    m_base_app.request_stop();
}

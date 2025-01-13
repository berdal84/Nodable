#include "ndbl/gui/Nodable.h"
#include "tools/core/TryCatch.h"

using namespace tools;
using namespace ndbl;

int main(int argc, char *argv[])
{
    TOOLS_try
    {
        Nodable app;
        app.init();
        while ( !app.should_stop() )
        {
            app.update();
            app.draw();
        }
        app.shutdown();
    }
    TOOLS_catch
    return 0;
}

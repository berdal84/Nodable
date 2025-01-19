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
        app.run();
        app.shutdown();
    }
    TOOLS_catch
    return 0;
}

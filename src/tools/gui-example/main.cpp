#include "AppExample.h"
#include "tools/core/TryCatch.h"

using namespace tools;

int main(int argc, char *argv[])
{
    TOOLS_try
    {
        // Instantiate the application using the predefined configuration
        AppExample app;
        app.init();
        app.run();
        app.shutdown();
    }
    TOOLS_catch
    return 0;
}


#include "fixtures/gui.h"
#include "tools/gui/Config.h"

typedef ::testing::Gui Gui_App;
using namespace ndbl;

TEST_F(Gui_App, constructor)
{
    Nodable app;
}

TEST_F(Gui_App, init_shutdown)
{
    Nodable app;
    app.init();
    app.shutdown();
}

TEST_F(Gui_App, update)
{
    Nodable app;
    app.init();
    app.update();
    app.shutdown();
}

TEST_F(Gui_App, loop_count_1)
{
    Nodable app;
    app.init();
    loop_count(app, 1);
    app.shutdown();
}

TEST_F(Gui_App, loop_duration_5s)
{
    Nodable app;
    app.init();
    loop_for_x_sec( app, 5.0 );
    app.shutdown();
}

TEST_F(Gui_App, new_file)
{
    Nodable app;
    app.init();
    app.get_view()->show_splashscreen(false);
    app.new_file();
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__new_file__0.png");
    app.shutdown();
}

TEST_F(Gui_App, open_file)
{
    Nodable app;
    app.init();
    app.get_view()->show_splashscreen(false);
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_file__0.png");
    EXPECT_TRUE(app.open_asset_file("./assets/examples/arithmetic.cpp"));
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_file__1.png");
    app.shutdown();
}

TEST_F(Gui_App, close_file)
{
    Nodable app;
    app.init();
    app.get_view()->show_splashscreen(false);
    File* file = app.open_asset_file("./assets/examples/arithmetic.cpp");
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__close_file__0.png");
    app.close_file( file );
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__close_file__1.png");
    app.shutdown();
}

TEST_F(Gui_App, open_examples)
{
    Nodable app;
    app.init();
    app.get_view()->show_splashscreen(false);
    app.update();
    save_screenshot(app, "TEST_Gui_App__open_examples__0.png");
    EXPECT_TRUE(app.open_asset_file("./assets/examples/arithmetic.cpp"));
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_examples__1.png");
    EXPECT_TRUE(app.open_asset_file("./assets/examples/for-loop.cpp"));
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_examples__2.png");
    EXPECT_TRUE(app.open_asset_file("./assets/examples/if-else.cpp"));
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_examples__3.png");
    EXPECT_TRUE(app.open_asset_file("./assets/examples/multi-instructions.cpp"));
    loop_for_x_sec( app, 1.0 );
    save_screenshot(app, "TEST_Gui_App__open_examples__4.png");
    app.shutdown();
}




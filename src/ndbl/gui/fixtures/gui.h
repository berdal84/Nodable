#include <gtest/gtest.h>

#include "ndbl/gui/Nodable.h"
#include "ndbl/gui/NodableView.h"
#include "tools/core/System.h"

#include <thread>

#ifdef NDBL_GUI_TEST_HUMAN_SPEED
#define SLEEP_FOR_HUMAN( delay_in_ms ) std::this_thread::sleep_for( std::chrono::milliseconds( delay_in_ms ) );
#else
#define SLEEP_FOR_HUMAN( delay_in_ms )
#endif

namespace testing
{
    class Gui: public Test {
    public:
        Gui()
        {
            // Override app label with test name
            const TestInfo* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            std::string label = "Test | ";
            label += test_info->test_suite_name();
            label += " - ";
            label += test_info->name();
        }

        /**
         * run some loops for a given iteration count
         */
        static void loop_count(ndbl::Nodable & app, size_t iteration_count, double sleep_in_sec = 1.0)
        {
            for(size_t i = 0; i < iteration_count; ++i)
            {
                EXPECT_NO_THROW(app.update());
                EXPECT_NO_THROW(app.draw());
            }
            SLEEP_FOR_HUMAN((long)(1000.0 * sleep_in_sec));
        }

        /**
         * run some loops for a given duration
         */
        static void loop_for_x_sec(ndbl::Nodable & app, double duration_in_sec)
        {
            auto start = std::chrono::system_clock::now();
            auto end   = std::chrono::system_clock::now();
            size_t iteration = 0;
            while((std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < (long long)(duration_in_sec * 1000.f)))
            {
                end = std::chrono::system_clock::now();
                TOOLS_LOG(tools::Verbosity_Message, "Test", "Loop iteration %llu (time: %0.1f/%0.1f sec)\n"
                , iteration
                , float(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f
                , duration_in_sec);
                EXPECT_NO_THROW(app.update());
                EXPECT_NO_THROW(app.draw());
                ++iteration;
            }
        }

        void save_screenshot(ndbl::Nodable & app, const char* relative_path)
        {
            TOOLS_LOG(tools::Verbosity_Message, "Test", "Taking screenshot ...\n");
            auto path = tools::Path::get_executable_path().parent_path() / "screenshots" / relative_path;
            if (!tools::Path::exists(path.parent_path()))
            {
                tools::Path::create_directories(path.parent_path());
            }
            app.get_view()->save_screenshot(path);
        }
    };
}
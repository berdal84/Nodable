#include <gtest/gtest.h>

#include <ndbl/gui/App.h>
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
            ndbl::Settings::get_instance().fw_app_view.app_window_label = label;
        }

        /**
         * run some loops for a given iteration count
         */
        void loop_count(ndbl::App& app, size_t iteration_count, double sleep_in_sec = 1.0)
        {
            for(int i = 0; i < iteration_count; ++i)
            {
                EXPECT_NO_THROW(app.update());
                EXPECT_NO_THROW(app.draw());
            }
            SLEEP_FOR_HUMAN((long)(1000.0 * sleep_in_sec));
        }

        /**
         * run some loops for a given duration
         */
        void loop_duration(ndbl::App& app, double duration_in_sec)
        {
            auto start = std::chrono::system_clock::now();
            auto end   = std::chrono::system_clock::now();
            size_t iteration = 0;
            while((std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < duration_in_sec))
            {
                end = std::chrono::system_clock::now();
                LOG_MESSAGE("Test", "Loop iteration %llu (time: %0.1f/%0.1f sec)\n"
                , iteration
                , float(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f
                , duration_in_sec);
                EXPECT_NO_THROW(app.update());
                EXPECT_NO_THROW(app.draw());
                ++iteration;
            }
        }
    };
}
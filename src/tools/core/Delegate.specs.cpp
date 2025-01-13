#include "Delegate.h"
#include "tools/core/log.h"
#include <gtest/gtest.h>

using namespace tools;

TEST(SimpleDelegate, default_constructor )
{
    SimpleDelegate d;
}

TEST(SimpleDelegate, default_call )
{
    SimpleDelegate d;
    d.call();
}

TEST(Delegate, void_no_args__on_classes )
{
    static bool success = false;

    class MyClass
    {
    public:
        void method() { success = true; };
    };

    MyClass obj;
    auto d = SimpleDelegate::from_method<&MyClass::method>(&obj);
    d.call();

    EXPECT_TRUE(success);
}

TEST(Delegate, void_no_args__on_structs )
{
    struct MyStruct
    {
        bool ok = false;
        void set_ok() { ok = true; };
    };

    MyStruct obj;
    auto d = SimpleDelegate::from_method<&MyStruct::set_ok>(&obj);
    d.call();
    EXPECT_TRUE(obj.ok);
}

TEST(Delegate, bind )
{
    struct MyStruct
    {
        bool ok = false;
        void set_ok() { ok = true; };
    };

    MyStruct obj;
    auto d = SimpleDelegate::from_method<&MyStruct::set_ok>(nullptr);
    d.bind(&obj);
    d.call();
    EXPECT_TRUE(obj.ok);
}
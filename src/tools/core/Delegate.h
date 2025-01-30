#pragma once
#include <functional>
#include "tools/core/assertions.h"
#include "tools/core/reflection/FunctionTraits.h"

namespace tools
{
    template<typename FunctionT>
    struct Delegate;

    //
    // struct Delegate is able to wrap a method and call it later on a given object_ptr
    // It does not rely on std::function, and is made from this code https://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates
    //
    // ex:
    // Delegate<void> d = Delegate<void>::from_method<&MyClass::my_method>( my_class_instance_ptr );
    // d.call();
    //
    // or
    //
    // Delegate<void> d = Delegate<void>::from_method<&MyClass::my_method>();
    // b.bind(my_class_instance_ptr);
    // d.call();
    //
    template<typename R, typename ...Args>
    struct Delegate<R(Args...)>
    {
        using FunctionPtrT = R(*)(void*, Args...);

        Delegate()
        : _m_function_ptr(&_null_function)
        , _m_object_ptr(nullptr)
        {}

        const void* object_ptr() const
        {
            return _m_object_ptr;
        }
        
        bool callable() const
        {
            return _m_object_ptr != nullptr
                && _m_function_ptr != &_null_function;
        }

        void bind(void* object_ptr)
        {
            _m_object_ptr = object_ptr;
        }

        R call(Args... args) const
        {
            return (*_m_function_ptr)(_m_object_ptr, args...);
        }

        bool operator==(const Delegate& other) const
        {
            return
                _m_object_ptr == other._m_object_ptr &&
                _m_function_ptr == other._m_function_ptr;
        }

        template<auto TMethod>
        requires std::is_member_function_pointer_v<decltype(TMethod)>
        static Delegate from_method(void* object_ptr)
        {
            using T = typename FunctionTrait<decltype(TMethod)>::class_t;
            Delegate delegate;
            delegate._m_object_ptr = object_ptr;
            delegate._m_function_ptr = &_method_caller<T, TMethod>; // <-- get address of a static function able to call the method
            return delegate;
        }

    private:
        void*        _m_object_ptr;
        FunctionPtrT _m_function_ptr;

        static R _null_function(void*, Args...) // Act like a default function, simply returns a default return value
        {
            if constexpr ( !std::is_void_v<R> )
                return {};
            return;
        }

        template <class TClass,  R(TClass::*TMethod)(Args...)>
        static R _method_caller(void* ptr, Args... args)
        {
            TClass* object_ptr = static_cast<TClass*>(ptr);
            return (object_ptr->*TMethod)(args...); // The trick is here, the method IS A TYPE!
        }
    };

    // define few alias
    using SimpleDelegate = Delegate<void()>;
}
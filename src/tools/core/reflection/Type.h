#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <typeinfo>
#include <vector>

#include "FunctionTraits.h"
#include "TypeRegister.h"
#include "tools/core/assertions.h"

// add this macro to a class declaration to enable reflection on it.
// Must have a parent class having REFLECT_BASE_CLASS macro.
#define DECLARE_REFLECT_EX( VIRTUAL, OVERRIDE ) \
    VIRTUAL const ::tools::ClassDescriptor* get_class() const OVERRIDE \
    { return ::tools::type::get_class(this); }

#define DECLARE_REFLECT          DECLARE_REFLECT_EX(        ,          )
#define DECLARE_REFLECT_virtual  DECLARE_REFLECT_EX( virtual,          )
#define DECLARE_REFLECT_override DECLARE_REFLECT_EX(        , override )

namespace tools
{
    // forward declarations
    class FunctionDescriptor;
    class IInvokable;
    class IInvokableMethod;
    class TypeDescriptor;
    class ClassDescriptor;
    class Operator;

    struct any{};  // Any type (like TypeScript's)
    struct unknown{}; // Unknown type (like TypeScript's)
    struct null{}; // Absence of type

    // Return true if T is reflected
    template<class T, typename GET_CLASS = decltype(&T::get_class)>
    constexpr bool IsReflectedClass = std::is_member_function_pointer_v<GET_CLASS>;

    /**
     * @struct Removes a pointer from a given type PointerT
     * @example @code
     * using _Class = tools::remove_pointer<Class*>::type; // _Class == Class
     */
    template<typename PointerT>
    struct remove_pointer
    {
        // PointerT without pointer (ex: void* => void, MyClass* => MyClass)
        using type = typename std::remove_pointer< typename std::decay<PointerT>::type>::type;
        constexpr static const char* name() { return typeid(type).name(); };
    };

    typedef int TypeFlags;
    enum TypeFlag_ : int
    {
        TypeFlag_NONE              = 0,
        TypeFlag_IS_CLASS          = 1 << 0,
        TypeFlag_IS_CONST          = 1 << 1,
        TypeFlag_IS_POINTER        = 1 << 2,
        TypeFlag_IS_MEMBER_PTR     = 1 << 3,
        TypeFlag_HAS_PARENT        = 1 << 5,
        TypeFlag_HAS_CHILD         = 1 << 6,
        TypeFlag_IS_INTEGRAL       = 1 << 7,
        TypeFlag_IS_FLOATING_POINT = 1 << 8
    };

    // Type utilities
    namespace type
    {
        bool               is_implicitly_convertible(const TypeDescriptor* _src, const TypeDescriptor* _dst);
        bool               equals(const TypeDescriptor* left, const TypeDescriptor* right);
        const TypeDescriptor*    any();
        const TypeDescriptor*    null();

        template<typename T> std::type_index    get_id();
        template<typename T> std::type_index    get_primitive_id();
        template<typename T> const char*        get_compiler_name();
        template<typename T> TypeFlags          get_flags();
        template<typename T> const TypeDescriptor*    get();
        template<typename T> const ClassDescriptor*   get_class(T* ptr);
        template<typename T> const ClassDescriptor*   get_class();
        template<typename T> TypeDescriptor*          create(const char* _name = "");
        template<typename T> const TypeDescriptor*    get(T value) { return get<T>(); }
    };

    /**
     * @class TypeDesc (type descriptor) holds meta data relative to a given type.
     *
     * @example @code
     * const TypeDesc* t = type::get<int>();
     * assert( t->is_ptr() == false );
     */
    class TypeDescriptor
    {
        friend TypeRegister;
    public:
        TypeDescriptor()
        : m_id(std::type_index(typeid(null))), m_primitive_id( std::type_index(typeid(null)) ) {}
        TypeDescriptor(std::type_index _id, std::type_index _primitive_id)
        : m_id(_id), m_primitive_id(_primitive_id) {}

        virtual ~TypeDescriptor() {};

        template<class T>
        static TypeDescriptor* create(const char* _name);
        std::type_index           id() const { return m_id; }
        const char*               compiler_name() const { return m_compiler_name; };
        const char*               name() const { return m_name.c_str(); };
        bool                      is_class() const { return m_flags & TypeFlag_IS_CLASS; }
        bool                      any_of(std::vector<const TypeDescriptor*> args)const;
        bool                      has_parent() const { return m_flags & TypeFlag_HAS_PARENT; }
        bool                      is_ptr() const { return m_flags & TypeFlag_IS_POINTER; }
        bool                      is_const() const { return m_flags & TypeFlag_IS_CONST; }
        bool                      is_integer() const { return m_flags & TypeFlag_IS_INTEGRAL; }
        bool                      is_floating_point() const { return m_flags & TypeFlag_IS_FLOATING_POINT; }
        bool                      equals(const TypeDescriptor* other) const { return type::equals(this, other); }
        template<typename T>
        bool                      is() const;
        bool                      is_implicitly_convertible(const TypeDescriptor* _dst ) const;
    protected:
        std::string m_name;
        const char* m_compiler_name = nullptr;
        TypeFlags   m_flags         = TypeFlag_NONE;
        std::type_index m_primitive_id; // ex: int
        std::type_index m_id;           // ex: int**, int*
    };

    /*
     * Simple object to store a named function argument
     */
    struct FuncArg
    {
        const TypeDescriptor* type;
        bool            pass_by_ref;
        std::string     name;
    };

    /*
     * Class to store a function signature.
     * We can check if two function signature are matching using this->match(other)
     */
    class FunctionDescriptor : public TypeDescriptor
    {
    public:

        template<typename T> static FunctionDescriptor* create(const char* _name);

        FunctionDescriptor() = default;
        bool                           is_exactly(const FunctionDescriptor*)const;
        bool                           is_compatible(const FunctionDescriptor*)const;
        const char*                    get_identifier()const { return m_name.c_str(); };
        FuncArg&                       arg_at(size_t i) { return m_argument[i]; }
        const FuncArg&                 arg_at(size_t i) const { return m_argument[i]; }
        std::vector<FuncArg>&          arg() { return m_argument;};
        const std::vector<FuncArg>&    arg()const { return m_argument;};
        size_t                         arg_count() const { return m_argument.size(); }
        const TypeDescriptor*          return_type() const { return m_return_type; }
        void                           set_return_type(const TypeDescriptor* _type) { m_return_type = _type; };
        template<typename T> void init(const char* _name);
        template<int ARG_INDEX, typename ArgsAsTuple>
        void                           push_nth_arg();
        template<typename ...Args>
        void                           push_args();
        void                           push_arg(const TypeDescriptor* _type, bool _pass_by_ref = false);
        bool                           has_arg_with_type(const TypeDescriptor*)const;
    private:
        std::vector<FuncArg>   m_argument;
        const TypeDescriptor*  m_return_type = type::null();
    };

    /**
     * @class ClassDesc (class descriptor) holds meta data relative to a given class.
     *
     * @example @code
     * const TypeDesc* class_desc = type::get<std::string>();
     * assert( class_desc->is_class() );
     */
    class ClassDescriptor : public TypeDescriptor
    {
        friend TypeRegister;
    public:
        ClassDescriptor() = default;
        ~ClassDescriptor();

        template<class T>
        static ClassDescriptor* create(const char* _name);

        void                      add_parent(std::type_index _parent);
        void                      add_child(std::type_index _child);
        void                      add_static(const char* _name, const IInvokable*);
        void                      add_method(const char* _name, const IInvokableMethod*);
        const std::unordered_set<const IInvokable*>&       get_statics()const { return m_static_methods; }
        const std::unordered_set<const IInvokableMethod*>& get_methods()const { return m_methods; }
        const IInvokable*         get_static(const char* _name) const;
        const IInvokableMethod*   get_method(const char* _name) const;
        bool                      is_child_of(std::type_index _possible_parent_id, bool _selfCheck = true) const;
        template<class T>
        bool               is_child_of() const { return is_child_of(std::type_index(typeid(T)), true); }
        template<class T>
        bool               is_not_child_of() const { return !is_child_of<T>(); }

    protected:
        std::unordered_set<std::type_index>                      m_parents;
        std::unordered_set<std::type_index>                      m_children;
        std::unordered_set<const IInvokable*>                    m_static_methods;
        std::unordered_map<std::string, const IInvokable*>       m_static_methods_by_name;
        std::unordered_set<const IInvokableMethod*>              m_methods;
        std::unordered_map<std::string, const IInvokableMethod*> m_methods_by_name;
    };

    template<typename T>
    std::type_index type::get_id()
    { return std::type_index(typeid(T)); }

    template<typename T>
    std::type_index type::get_primitive_id()
    { return type::get_id<typename remove_pointer<T>::type>(); }

    template<typename T>
    const char* type::get_compiler_name()
    { return typeid(T).name(); }

    template<int N, typename ArgsAsTuple>
    void FunctionDescriptor::push_nth_arg()
    {
        using NTH_ARG = std::tuple_element_t<N, ArgsAsTuple>;
        push_arg( type::get<NTH_ARG>(), std::is_reference_v<NTH_ARG> );
    }

    template<typename ...Args>
    void FunctionDescriptor::push_args()
    {
        constexpr size_t ARG_COUNT = std::tuple_size_v<Args...>;
        static_assert(ARG_COUNT <= 8, "maximum 8 arguments can be pushed at once");

        // note: I duplicate instead of using template recursion hell. :)

        if constexpr (ARG_COUNT > 0 ) push_nth_arg<0, Args...>();
        if constexpr (ARG_COUNT > 1 ) push_nth_arg<1, Args...>();
        if constexpr (ARG_COUNT > 2 ) push_nth_arg<2, Args...>();
        if constexpr (ARG_COUNT > 3 ) push_nth_arg<3, Args...>();
        if constexpr (ARG_COUNT > 4 ) push_nth_arg<4, Args...>();
        if constexpr (ARG_COUNT > 5 ) push_nth_arg<5, Args...>();
        if constexpr (ARG_COUNT > 6 ) push_nth_arg<6, Args...>();
        if constexpr (ARG_COUNT > 7 ) push_nth_arg<7, Args...>();
    }

    template<typename T>
    bool TypeDescriptor::is() const
    { return type::equals(this, type::get<T>()); }

    template<typename T>
    TypeDescriptor* TypeDescriptor::create(const char* _name)
    {
        static_assert( std::is_class_v<T> == false );

        TypeDescriptor* descriptor = new TypeDescriptor(type::get_id<T>(), type::get_primitive_id<T>() );

        descriptor->m_name          = _name;
        descriptor->m_compiler_name = type::get_compiler_name<T>();
        descriptor->m_flags         = type::get_flags<T>();

        return descriptor;
    }

    template<typename T>
    ClassDescriptor* ClassDescriptor::create(const char* _name)
    {
        static_assert( std::is_class_v<T> );

        ClassDescriptor* class_desc = new ClassDescriptor();

        class_desc->m_id            = type::get_id<T>();
        class_desc->m_primitive_id  = type::get_primitive_id<T>();
        class_desc->m_name          = _name;
        class_desc->m_compiler_name = type::get_compiler_name<T>();
        class_desc->m_flags         = type::get_flags<T>();

        return class_desc;
    }

    template<typename T>
    FunctionDescriptor* FunctionDescriptor::create(const char* _name)
    {
        FunctionDescriptor* descriptor = new FunctionDescriptor();
        descriptor->init<T>(_name);
        return descriptor;
    }

    template<typename T>
    void FunctionDescriptor::init(const char* _name)
    {
        m_id            = type::get_id<T>();
        m_primitive_id  = type::get_primitive_id<T>();
        m_compiler_name = type::get_compiler_name<T>();
        m_flags         = type::get_flags<T>();
        m_return_type   = type::get<typename FunctionTrait<T>::result_t >();
        m_name          = _name;

        using Args = typename FunctionTrait<T>::args_t;
        if constexpr ( std::tuple_size_v<Args> != 0)
            push_args<Args>();
    }

    template<typename T>
    const ClassDescriptor* type::get_class()
    {
        static_assert( std::is_class_v<T> );
        return (const ClassDescriptor*)get<T>();
    }

    template<typename T>
    const ClassDescriptor* type::get_class(T* ptr)
    {
        static_assert( std::is_class_v<T> );
        return (const ClassDescriptor*)get<T>();
    }

    template<typename T>
    const TypeDescriptor* type::get()
    {
        auto id = type::get_id<T>();

        if ( TypeRegister::has(id) )
        {
            return TypeRegister::get(id);
        }

        TypeDescriptor* descriptor = type::create<T>();
        TypeRegister::insert(descriptor);

        return descriptor;
    }

    template<typename T>
    TypeFlags type::get_flags()
    {
        return  (TypeFlag_IS_POINTER    * std::is_pointer_v<T>)
              | (TypeFlag_IS_CONST      * std::is_const_v<T>)
              | (TypeFlag_IS_MEMBER_PTR * std::is_member_pointer_v<T>)
              | (TypeFlag_IS_INTEGRAL   * std::is_integral_v<T>)
              | (TypeFlag_IS_FLOATING_POINT * std::is_floating_point_v<T>)
              | (TypeFlag_IS_CLASS      * std::is_class_v<T>);
    }

    template<typename T>
    TypeDescriptor* type::create(const char* _name)
    {
        if constexpr ( std::is_member_function_pointer_v<T> || std::is_function_v<T>)
            return FunctionDescriptor::create<T>(_name);
        if constexpr ( std::is_class_v<T> )
            return ClassDescriptor::create<T>(_name);
        else
            return TypeDescriptor::create<T>(_name);
    }

    /**
     * Return if SourceClass extends PossiblyBaseClass
     */
    template<class PossiblyBaseClass, class SourceClass, bool self_check = true>
    bool extends(SourceClass* source_ptr)
    {
        // ensure both classes are reflected
        static_assert(IsReflectedClass<SourceClass>);
        static_assert(IsReflectedClass<PossiblyBaseClass>);

        // check if source_type is a child of possibly_base_class
        const ClassDescriptor* source_type         = source_ptr->get_class();
        const TypeDescriptor*  possibly_base_class = type::get<PossiblyBaseClass>();
        return source_type->is_child_of(possibly_base_class->id(), self_check );
    }

    template<class TargetClass>
    TargetClass* cast(TargetClass* source_ptr)
    { return source_ptr; }

    template<class TargetClass, class SourceClass>
    TargetClass* cast(SourceClass* source_ptr)
    {
        static_assert(!std::is_same_v<TargetClass, SourceClass>);

        if( extends<TargetClass>(source_ptr) )
        {
            return static_cast<TargetClass*>(source_ptr);
        }
        return nullptr;
    }

    template<class T, class>
    T* cast(T* source_ptr)
    {
        return source_ptr;
    }
}
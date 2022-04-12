#pragma once

#include <unordered_set>
#include <string>

#include "typeregister.h"

namespace Nodable
{
    struct any_t{};
    struct null_t{};
    using hash_code_t = size_t;

    template<typename T>
    struct unqualified
    {
        using  type = typename std::remove_pointer< typename std::decay<T>::type>::type;
        constexpr static const char* name() { return typeid(type).name(); };
        constexpr static size_t hash_code() { return typeid(type).hash_code(); };
    };

    template<typename T>
    struct is_class
    {
        static constexpr bool value =       std::is_class<typename unqualified<T>::type>::value
                                            && !std::is_same<any_t, T>::value
                                            && !std::is_same<null_t, T>::value;
    };

    class type
    {
        friend struct registration;
        friend struct typeregister;
    private:
        type() = default;
    public:
        ~type() = default;

        const char*               get_name() const { return m_name.c_str(); };
        std::string               get_fullname() const;
        size_t                    hash_code() const { return m_hash_code; }
        bool                      is_class() const { return m_is_class; }
        bool                      is_ptr() const;
        bool                      is_ref() const;
        bool                      is_const() const;
        bool                      is_child_of(type _possible_parent_class, bool _selfCheck = true) const;
        void                      add_parent(hash_code_t _parent);
        void                      add_child(hash_code_t _child);
        template<class T> inline bool is_child_of() const { return is_child_of(get<T>(), true); }
        template<class T> inline bool is_not_child_of() const { return !is_child_of(get<T>(), true); }

        static type               to_pointer(type);
        static bool               is_ptr(type);
        static bool               is_ref(type);
        static bool               is_implicitly_convertible(type _src, type _dst);

        friend bool operator==(const type& left, const type& right)
        {
            return left.m_hash_code == right.m_hash_code;
        }

        friend bool operator!=(const type& left, const type& right)
        {
            return left.m_hash_code != right.m_hash_code;
        }

        /** to get a type at compile time */
        template<typename T>
        static type get()
        {
            auto hash = Nodable::unqualified<T>::hash_code();
            NODABLE_ASSERT_EX( typeregister::has(hash), "type is not registered. use registration::push_xxx<T>()")
            type type = typeregister::get(hash);
            type.m_is_pointer   = std::is_pointer<T>::value;
            type.m_is_reference = std::is_reference<T>::value;
            type.m_is_const     = std::is_const<T>::value;
            return type;
        }

        template<typename T>
        static type create(const char* _name)
        {
            type type;
            type.m_name                  = _name;
            type.m_compiler_name         = Nodable::unqualified<T>::name();
            type.m_hash_code             = Nodable::unqualified<T>::hash_code();
            type.m_is_pointer            = std::is_pointer<T>::value;
            type.m_is_reference          = std::is_reference<T>::value;
            type.m_is_const              = std::is_const<T>::value;
            type.m_is_class              = Nodable::is_class<T>::value;

            return type;
        }

        /** to get a type ar runtime */
        template<typename T>
        static type get(T value) { return get<T>(); }

        static type any;
        static type null;
    protected:
        std::string m_name;
        std::string m_compiler_name;
        bool        m_is_class;
        bool        m_is_pointer;
        bool        m_is_reference;
        bool        m_is_const;
        hash_code_t m_hash_code;
        std::unordered_set<hash_code_t> m_parents;
        std::unordered_set<hash_code_t> m_children;
    };
}
#pragma once

#include <string>
#include <array>
#include <memory> // std::shared_ptr

#include "tools/core/assertions.h"
#include "tools/core/types.h"
#include "qword.h"
#include "Type.h"

namespace tools
{
    /**
     * @brief This class can hold several types such as: bool, double, std::string, etc.. (see m_data get_value)
     */
	class variant
    {
    public:
        variant();
        ~variant();

        variant(const variant& other);
        variant(const std::string& val);
        variant(const char* val);
        variant(double val);
        variant(i16_t val) ;
        variant(i32_t val);
        variant(bool val);
        variant(null val);

        void        set(void* ptr);
        void        set(const std::string& _value);
        void        set(const char* _value);
        void        set(null);
        void        set(double);
        void        set(bool);
        void        set(i16_t);
        void        set(i32_t);
        void        set(const variant&);

        const TypeDescriptor* get_type()const;
        bool        is_type(const TypeDescriptor*) const;
        void        change_type(const TypeDescriptor* _type);

        void        clear_data();
        const qword*data() const; // get ptr to underlying data (qword)

        template<typename T>
        T           to()const;
        variant&    operator=(const variant& other);
        explicit operator double&();
        explicit operator i32_t&();
        explicit operator i16_t&();
        explicit operator bool&();
        explicit operator std::string& ();
        explicit operator double() const;
        explicit operator i32_t() const;
        explicit operator i16_t() const;
        explicit operator bool() const;
        explicit operator std::string() const;
        explicit operator const char*() const;
        explicit operator void* () const;

        template<typename T>
        T& as() { return (T)*this; }

        template<typename T>
        T as() const { return (T)*this; }
    private:
        enum Type // Internal Type enum to speedup switch/cases
        {
            Type_null = 0,
            Type_any, // "similar" to TypeScript's any.
            Type_bool,
            Type_double,
            Type_i16,
            Type_i32,
            Type_string,
            Type_ptr
        };

        void change_type(Type new_type);
        void init_mem();
        void release_mem(); // undo init_mem()
        bool is_mem_initialized() const;

        static Type                   type_to_enum(const TypeDescriptor*) ;
        static const tools::TypeDescriptor* enum_to_type(Type) ;

        typedef int Flags;
        enum Flag_
        {
            Flag_NONE                       = 0,
            Flag_OWNS_HEAP_ALLOCATED_MEMORY = 1 << 0, // True when dynamically allocated memory is owned by this variant (ex: a std::string*)
            Flag_ALLOWS_TYPE_CHANGE         = 1 << 1  // True if variant's type can change over time, by default its strict (type can be set once).
        };

        Type          m_type  = Type_any;
        Flags         m_flags = Flag_NONE;
        qword         m_data  = {};
    };
}
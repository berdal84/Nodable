#pragma once

#include <cassert>
#include <cstring> // for memcpy
#include <memory> // for std::move
#include <cstdarg> // va_list, va_start, va_end
#include <cstdio>
#include "types.h"

namespace tools
{
    enum class alloc_strategy {
        NEXT_ALLOC_USE_HEAP, // Currently allocated on stack, next alloc will be on the heap
        HEAP                 // Currently allocated on the heap, next alloc will be on the heap
    };

    template<typename CharT = char>
    class basic_string {

    protected: CharT* m_ptr;          // Pointer to the buffer (static or dynamic)
    protected: size_t m_length;       // String length (excluding terminal string)
    protected: size_t m_capacity;     // Buffer size - 1
    protected: alloc_strategy m_alloc_strategy;

    public:
        basic_string()
            : m_alloc_strategy(alloc_strategy::HEAP)
            , m_capacity(0)
            , m_length(0)
            , m_ptr(nullptr)
        {}

        explicit basic_string(const CharT* str)
            : m_alloc_strategy(alloc_strategy::HEAP)
            , m_length(strlen(str))
            , m_capacity(0)
            , m_ptr(nullptr)
        {
            if( m_length > 0 )
            {
                m_ptr = expand_capacity_to_fit(m_length);
                memcpy(m_ptr, str, m_length);
                m_ptr[m_length] = 0;
            }
        }

        basic_string(const basic_string& other)
            : basic_string(other.c_str())
        {}

        basic_string(basic_string&& other) noexcept
            : m_alloc_strategy(alloc_strategy::NEXT_ALLOC_USE_HEAP)
            , m_length(0)
            , m_capacity(0)
            , m_ptr(nullptr)
        {
            *this = std::move(other);
        }

        basic_string& operator=(basic_string&& other) noexcept
        {
            if ( this == &other )
                return *this;

            if( m_alloc_strategy == alloc_strategy::HEAP )
            {
                delete[] m_ptr;
                m_ptr = other.m_ptr;
                m_length = other.m_length;
                m_capacity = other.m_capacity;
            }
            else
            {
                append(other); // may use heap or stack depending on capacity
            }

            other.m_length = 0;
            other.m_capacity = 0;
            other.m_ptr = nullptr;

            return *this;
        }

        basic_string& operator=(const basic_string& other)
        {
            if( this == &other)
                return *this;

            if( m_capacity < other.m_capacity)
                m_ptr = expand_capacity_to_fit(other.m_length);

            memcpy(m_ptr, other.m_ptr, other.m_length);
            m_length = other.m_length;
            m_capacity = other.m_capacity;
            m_ptr[m_length] = 0;
            return *this;
        }

    protected:
        basic_string(CharT* data, size_t capacity, size_t length, alloc_strategy strategy = alloc_strategy::NEXT_ALLOC_USE_HEAP)
            : m_alloc_strategy(strategy)
            , m_capacity(capacity )
            , m_length(length)
            , m_ptr(data)
        {}


    public:
        ~basic_string()
        {
            if( m_alloc_strategy == alloc_strategy::HEAP && m_ptr != nullptr)
            {
                delete[] m_ptr;
            }
        }

        bool heap_allocated() const
        { return m_alloc_strategy == alloc_strategy::HEAP; }

        const char* data() const
        { return m_ptr != nullptr ? const_cast<const char*>(m_ptr) : ""; }

        const char* c_str() const
        { return data(); }

        basic_string& append(const CharT* str, size_t length)
        {
            if( m_capacity < m_length + length )
            {
                m_ptr = expand_capacity_to_fit(m_length + length);
            }
            memcpy(m_ptr + m_length, str, length);
            m_length += length;
            m_ptr[m_length] = 0;
            return *this;
        }
        basic_string& append(const basic_string& str)
        { return append(str.m_ptr, str.m_length); }

        basic_string& append(const CharT* str)
        { return append(str, strlen(str)); }

        template<typename ...Args>
        size_t append_fmt(const char* _format, Args...args )
        { return m_length = snprintf(m_ptr+m_length, m_capacity-m_length, _format, args... ); }

        size_t append_fmt(const char* _str )
        { return m_length = snprintf(m_ptr+m_length, m_capacity-m_length, "%s", _str ); }

        /** provided to easily switch to/from std::string */
        basic_string& push_back(CharT str)
        { return append(&str, 1); }

        size_t capacity() const
        { return m_capacity; }

        size_t length() const
        { return m_length; }

        bool is_empty() const
        { return m_length == 0; }

        void clear()
        {
            m_length = 0;
            if( m_ptr != nullptr) m_ptr[0] = 0;
        }

        bool equals(const basic_string& other) const {
            return m_length == other.m_length && strcmp(c_str(), other.c_str()) == 0;
        }

    private:
        /**
         * Expand the buffer to the closest power of two of the desired set_size.
         */
        CharT* expand_capacity_to_fit(size_t desired_capacity)
        {
            assert(desired_capacity > m_capacity );

            // compute the next highest power of 2 of 64-bit

            // based on https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
            size_t new_buf_size = desired_capacity;
            //new_buf_size--; // capacity is buffer_size - 1
            new_buf_size |= new_buf_size >> 1;
            new_buf_size |= new_buf_size >> 2;
            new_buf_size |= new_buf_size >> 4;
            new_buf_size |= new_buf_size >> 8;
            new_buf_size |= new_buf_size >> 16;

            new_buf_size++;

            CharT* new_ptr = new CharT[new_buf_size];

            if( m_ptr )
            {
                memcpy(new_ptr, m_ptr, m_length+1); // We only copy the string + null char
            }

            if (m_alloc_strategy == alloc_strategy::HEAP)
            {
                delete[] m_ptr;
            }
            else
            {
                m_alloc_strategy = alloc_strategy::HEAP;
            }
            m_capacity = new_buf_size - 1;

            return new_ptr;
        }
    };

    /**
     * Stack allocated string.
     * Switches to dynamic allocations when stack buffer is too small.
     *
     * Buffer set_size and string length are stored in an unsigned integer (1 byte)
     */
    template<size_t STATIC_BUF_SIZE, typename CharType = char>
    class inline_string : public basic_string<CharType> {
    private:
        static_assert(STATIC_BUF_SIZE >= 8);
        CharType m_static_buf[STATIC_BUF_SIZE]; // Static buffer

    public:
        inline_string(): basic_string<CharType>(m_static_buf, STATIC_BUF_SIZE-1, 0)
        { m_static_buf[0] = '\0'; }

        inline_string(CharType *str, size_t length) : basic_string<CharType>(m_static_buf, STATIC_BUF_SIZE-1, 0)
        { this->append(str, length); }

        inline_string(const CharType *str) : inline_string(const_cast<char*>(str), strlen(str))
        {}

        inline_string(const basic_string<CharType>& other): inline_string()
        {  this->append(other.c_str(), other.length()); }
    };

    // Define some aliases

    using string    = basic_string<char>;
    using string8   = inline_string<8>;
    using string16  = inline_string<16>;
    using string32  = inline_string<32>;
    using string64  = inline_string<64>;
    using string128 = inline_string<128>;
    using string256 = inline_string<256>;
    using string512 = inline_string<512>;
}
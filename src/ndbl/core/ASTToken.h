#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <cstring>

#include "tools/core/string.h"
#include "tools/core/reflection/reflection"
#include "tools/core/assertions.h"
#include "ASTToken_t.h"

namespace ndbl
{
    /**
     * A token is a view over a portion of parsed string.
     *
     * The text viewed by a Token can be split in 3 parts:
     * - prefix
     * - word
     * - suffix
     *
     * For example, if we parsed the text "   my_var ", those part will contain:
     * - prefix: "   "
     * - word:   "my_var"
     * - suffix: " "
     *
     * Data can can be external or internal:
     * Token's buffer can be either external or internal, when the string is modified (ex: by pushing chars in the suffix),
     * the Token switch into external mode to avoid writing on non owned memory. This allows to work with external const char*.
     * Ideally, all the Token from a given parsing should share the same buffer until user modify something.
     */
	struct ASTToken
	{
        size_t  m_index = 0; // in the TokenRibbon
        ASTToken_t m_type;

        struct BimodalBuffer
        {
            typedef int Flags;
            enum Flags_
            {
                Flags_NONE     = 0,
                Flags_INTERN   = 1,
                Flags_READONLY = 1 << 1
            };

            union {
                char*        extern_buf;
                std::string* intern_buf;
            } /* data */;
            size_t offset; // to offset token start from data's address
            Flags  _flags;

            ~BimodalBuffer();
            void         delete_intern_buf();
            void         switch_to_intern_buf(size_t size);

            char*        data()  const { return intern() ? intern_buf->data() : extern_buf; }
            char*        begin() const { return data() + offset; }
            bool         intern() const { return _flags & BimodalBuffer::Flags_INTERN; }
            bool         readonly() const { return _flags & BimodalBuffer::Flags_READONLY; }
        } m_buffer = {nullptr, 0, BimodalBuffer::Flags_NONE }; // external nullptr

        size_t      m_prefix_len       = 0;
        size_t      m_word_len         = 0;
        size_t      m_suffix_len       = 0;

        explicit ASTToken()
        : m_type(ASTToken_t::none)
        {}

        ASTToken(ASTToken_t type)
        : m_type( type )
        {}

        ASTToken(ASTToken_t type, const std::string& word )
        : m_type( type )
        {
            word_replace( word.c_str() );
        }

        ASTToken(ASTToken_t type, const char* const word)
        : m_type(type)
        {
            m_buffer.extern_buf = const_cast<char*>(word),
            m_word_len        = strlen(word);
        }

        ASTToken(ASTToken_t type,
                 char*   buffer,
                 size_t  size)
        : m_type(type)
        , m_word_len(size)
        , m_buffer({ const_cast<char*>(buffer), 0, BimodalBuffer::Flags_NONE}) // external
        {
        }

        ASTToken(ASTToken_t     type,
                 char* const buffer,
                 size_t      offset,
                 size_t      size)
        : m_type(type)
        , m_word_len(size)
        , m_buffer({const_cast<char*>(buffer), offset, BimodalBuffer::Flags_NONE}) // external
        {
        }

        ASTToken(const ASTToken& other)
            : m_type(other.m_type)
            , m_index(other.m_index)
            , m_prefix_len(other.m_prefix_len)
            , m_word_len(other.m_word_len)
            , m_suffix_len(other.m_suffix_len)
            , m_buffer(other.m_buffer)
        {
            VERIFY( !other.m_buffer.intern(), "Can't create a Token from an owned const char*");
        }

        ASTToken(ASTToken&& other);

        explicit operator bool () const
        { return m_type != ASTToken_t::none; }

        ASTToken& operator=(const ASTToken& other);
        ASTToken& operator=(ASTToken&&) noexcept;
        ~ASTToken();

        void        clear();
        bool has_buffer()const { return m_buffer.data() != nullptr; }

        // Convert token portion to string
        // Warning: this costs an allocation, use xxx_ptr() and xxx_size() when possible.

        std::string string() const;
        std::string prefix_to_string()const;
        std::string word_to_string()const;
        std::string suffix_to_string()const;

        // Get offset/positions

        size_t      offset() const { return m_buffer.offset; }
        void        set_offset(size_t new_offset);

        void        prefix_reset(size_t size = 0); // word won't change;
        void        prefix_begin_grow(size_t l_amount); // word won't change
        void        prefix_end_grow(size_t r_amount); // word will change;

        void        suffix_reset(size_t size = 0); // word won't change
        void        suffix_end_grow(size_t r_amount); // word won't change
        void        suffix_begin_grow(size_t l_amount); // word will change;

        void        word_move_begin(int amount);
        void        word_move_end(int amount);

        void        reset_lengths(); // buffer and offset won't change

        // Get token portion addresses

        char*       begin() const  { return m_buffer.begin(); } // token's start address
        char*       prefix() const { return begin(); }
        char*       word() const   { return begin() + m_prefix_len; /* word pos is absolute */ };
        char*       suffix() const { return begin() + m_prefix_len +  m_word_len; }
        char*       end() const    { return begin() + m_prefix_len +  m_word_len + m_suffix_len; }

        // Get token portion sizes

        size_t      length() const     { return m_prefix_len + m_word_len + m_suffix_len; }
        size_t      prefix_len() const { return m_prefix_len; }
        size_t      word_len() const   { return m_word_len; }
        size_t      suffix_len() const { return m_suffix_len; }

        bool        is_keyword_type() { return ndbl::is_a_type_keyword(m_type); } // Check if whether this token is a keyword type
        void        take_prefix_suffix_from(ASTToken *source); // Transfer the prefix and suffix of a given token to this token
        void        set_external_buffer(char* buffer, size_t offset = 0, size_t size = 0, bool external_only = false);
        std::string json()const;
        bool        empty() const { return length() == 0; }
        void        suffix_push_back(const char* str);
        void        prefix_push_front(const char *str);
        void        word_replace(const char* new_word);
        static const ASTToken s_end_of_line;
        static const ASTToken s_end_of_instruction;
    };
}
static_assert(std::is_move_assignable_v<ndbl::ASTToken>, "Should be move assignable");
static_assert(std::is_move_constructible_v<ndbl::ASTToken>, "Should be move constructible");
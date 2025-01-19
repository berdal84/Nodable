#include "ASTToken.h"
#include <cstring>

using namespace ndbl;

const ASTToken ASTToken::s_end_of_line        = {ASTToken_t::ignore, "\n"};
const ASTToken ASTToken::s_end_of_instruction = {ASTToken_t::ignore, ";\n"};

ASTToken::ASTToken(ASTToken&& other)
{
    *this = std::move( other );
};

ASTToken::~ASTToken()
{
}

std::string ASTToken::json() const
{
    std::string result;
    result.reserve( length() );

    result.append("{ ");

    result.append("word: \"" );
    result.append(word(), word_len());
    result.append("\"" );

    result.append(", charIndex: " + std::to_string( (u64_t)word() - (u64_t) begin() ) );

    result.append(", prefix: \""  );
    result.append(prefix(), prefix_len());
    result.append("\""  );

    result.append(", suffix: \""  );
    result.append(suffix(), suffix_len());
    result.append("\""  );

    result.append(", token_type: \"" + std::to_string((int)m_type) + "\"");

    result.append(" }");

    return result;
}

void ASTToken::take_prefix_suffix_from(ASTToken* source)
{
    std::string word_copy = word_to_string();
    m_buffer.switch_to_intern_buf(length());

    // transfer prefix and suffix to this token, but keep the same word.
    // this operation requires the buffer to be owned

    m_prefix_len       = source->m_prefix_len;
    // m_word_len      = unchanged
    m_suffix_len       = source->m_suffix_len;

    // copy prefix from source
    if( source->m_prefix_len )
    {
        m_buffer.intern_buf->append(source->begin(), source->m_prefix_len);
    }

    // reassign word
    if( word_copy.length() )
    {
        m_buffer.intern_buf->append(word_copy.data(), word_copy.length());
    }

    // copy suffix from source
    if( source->m_suffix_len )
    {
        m_buffer.intern_buf->append(source->suffix(), source->m_suffix_len);
    }

    // Remove prefix and suffix on the source
    source->suffix_reset();
    source->prefix_reset();
}

void ASTToken::clear()
{
    m_index      = 0;
    m_type       = ASTToken_t::none;
    m_prefix_len = 0;
    m_word_len   = 0;
    m_suffix_len = 0;

    m_buffer.delete_intern_buf();
}

void ASTToken::set_external_buffer(char* buffer, size_t offset, size_t size, bool external_only )
{
    // here, we consider that the whole buffer will be into the "word" part, no suffix/prefix.

    m_buffer.delete_intern_buf();

    m_buffer._flags     = BimodalBuffer::Flags_READONLY * external_only;
    m_buffer.extern_buf = buffer;
    m_buffer.offset     = offset;
    m_prefix_len = 0;
    m_word_len   = size;
    m_suffix_len = 0;
}

std::string ASTToken::prefix_to_string()const
{
    if( has_buffer() && m_prefix_len )
    {
        return std::string{prefix(), m_prefix_len};
    }
    return {};
}

std::string ASTToken::word_to_string()const
{
    if( has_buffer() && m_word_len)
        return std::string{word(), m_word_len};
    return {};
}

std::string ASTToken::suffix_to_string()const
{
    if( has_buffer() && m_suffix_len )
        return std::string{suffix(), m_suffix_len};
    return {};
}

std::string ASTToken::string() const
{
    if ( has_buffer() )
        return { begin(), length() };
    return {};
}

ASTToken& ASTToken::operator=(ASTToken&& other) noexcept
{
    if( this == &other )
    {
        return *this;
    }

    m_buffer.delete_intern_buf();

    m_buffer     = other.m_buffer;
    m_prefix_len = other.m_prefix_len;
    m_word_len   = other.m_word_len;
    m_suffix_len = other.m_suffix_len;
    m_type       = other.m_type;
    m_index      = other.m_index;

    other.m_buffer     = {}; // otherwise it would destroy it

    return *this;
};

ASTToken& ASTToken::operator=(const ASTToken& other)
{
    if( this == &other) return *this;

    m_buffer.delete_intern_buf();

    m_index      = other.m_index;
    m_prefix_len = other.m_prefix_len;
    m_word_len   = other.m_word_len;
    m_suffix_len = other.m_suffix_len;
    m_type       = other.m_type;

    if( !other.m_buffer.intern() )
    {
        m_buffer.switch_to_intern_buf(other.length());
        m_buffer.intern_buf->append({other.begin(), other.length()});
    }
    else
    {
        m_buffer = other.m_buffer;
    }

    return *this;
}

void ASTToken::word_replace(const char* new_word)
{
    const size_t new_word_len = strlen(new_word);

    if( new_word_len == 0 )
        return;

    std::string prefix_copy = prefix_to_string();
    std::string suffix_copy = suffix_to_string();

    m_buffer.switch_to_intern_buf(m_prefix_len + new_word_len + m_suffix_len);

    m_buffer.intern_buf->clear();
    m_buffer.intern_buf->append(prefix_copy );
    m_buffer.intern_buf->append(new_word );
    m_buffer.intern_buf->append(suffix_copy );

    // m_prefix_len    = no change
    m_word_len         = new_word_len;
    // m_suffix_len    = no change
}

void ASTToken::prefix_push_front(const char* str)
{
    if (!m_buffer.intern())
    {
        size_t size = length();
        char*  data = begin();
        m_buffer.switch_to_intern_buf(size);
        m_buffer.intern_buf->append( data, size );
    }
    m_buffer.intern_buf->insert(0, str);
    m_prefix_len += strlen(str);
}

void ASTToken::suffix_push_back(const char* str)
{
    if (!m_buffer.intern())
    {
        size_t size = length();
        char*  data = begin();
        m_buffer.switch_to_intern_buf(size);
        m_buffer.intern_buf->append( data, size );
    }
    m_buffer.intern_buf->append(str);
    m_suffix_len += strlen(str);
}

void ASTToken::prefix_reset(size_t size )
{
    // Instead of erasing chars, we prefer to simply "move the cursor to the right"
    m_buffer.offset += m_prefix_len - size;
    m_prefix_len     = size;
}

void ASTToken::reset_lengths()
{
    m_prefix_len = m_word_len = m_suffix_len = 0;
}

void ASTToken::word_move_begin(int amount)
{
    m_prefix_len = (int)m_prefix_len + amount;
    m_word_len   = (int)m_word_len - amount;
}

void ASTToken::word_move_end(int amount)
{
    m_word_len   = (int)m_word_len + amount;
    m_suffix_len = (int)m_suffix_len - amount;
}

void ASTToken::set_offset(size_t pos)
{
    m_buffer.offset = pos;
}

void ASTToken::suffix_reset(size_t size)
{
    m_suffix_len = size;
}

void ASTToken::prefix_begin_grow(size_t l_amount)
{
    VERIFY( offset() >= l_amount, "Can't extend prefix above data's boundary");

    set_offset( offset() - l_amount ); // slide
    m_prefix_len += l_amount;
}

void ASTToken::suffix_end_grow(size_t size)
{
    suffix_reset(m_suffix_len + size);
}

void ASTToken::suffix_begin_grow(size_t l_amount)
{
    ASSERT( m_word_len >= l_amount );
    m_word_len   -= l_amount;
    m_suffix_len += l_amount;
}

void ASTToken::prefix_end_grow(size_t r_amount)
{
    ASSERT( r_amount <= m_word_len);
    m_prefix_len += r_amount;
    m_word_len   -= r_amount;
}

ASTToken::BimodalBuffer::~BimodalBuffer()
{
    delete_intern_buf();
}

void ASTToken::BimodalBuffer::switch_to_intern_buf(size_t size)
{
    ASSERT( !readonly() );

    // Initialize memory
    if ( !intern() )
    {
        intern_buf = new std::string();
        _flags     = BimodalBuffer::Flags_INTERN;
        offset     = 0;
    }
    intern_buf->reserve(size);
}


void ASTToken::BimodalBuffer::delete_intern_buf()
{
    if( !intern() )
        return;

    VERIFY( !readonly(), "Can't delete readonly buffers" );
    delete intern_buf;

    _flags     = BimodalBuffer::Flags_NONE;
    offset     = 0;
    intern_buf = nullptr;
}


#include "ASTTokenRibbon.h"

#include "tools/core/log.h"
#include "tools/core/assertions.h"

#include "ASTToken_t.h"
#include "ASTToken.h"

using namespace ndbl;

ASTToken & ASTTokenRibbon::push(ASTToken &_token)
{
    _token.m_index = m_tokens.size();
    m_tokens.push_back(_token);
    return m_tokens.back();
}

std::string ASTTokenRibbon::to_string()const
{
    tools::string out;
    out.append(TOOLS_COLOR_DEFAULT);

    size_t buffer_size = 0;

    // get the total buffer sizes (but won't be exact, some token are serialized dynamically)
    for (const ASTToken& each_token : m_tokens)
        buffer_size += each_token.length();

    out.append("Logging token ribbon state:\n");
    out.append("___________[TOKEN RIBBON]_________\n");

    for (const ASTToken& token : m_tokens)
    {
        tools::string512 line;
        
        if ( token.m_index == 0 )
        {
            line.append("B"); // begin
        }
        else if ( token.m_index == m_tokens.back().m_index )
        {
            line.append("E"); // end
        }
        else
        {
            line.append("|"); // default
        }

        if ( !m_transaction.empty()
              && token.m_index >= m_transaction.top()
              && token.m_index <= m_cursor )
        {
            line.append("T"); // transaction
        }
        else
        {
            line.append("."); // no transaction
        }

        const std::string word = token.word_to_string();
        line.append_fmt("%5zu) \"%s\"", token.m_index, word.c_str() );
      
        if ( token.m_index == m_cursor )
        {
            line.append(" [c]"); // current
        }
        
        out.append(line.c_str());
        out.append("\n");
    }

    return out.c_str();
}

ASTToken ASTTokenRibbon::eat_if(ASTToken_t expectedType)
{
    if (can_eat() && peek().m_type == expectedType )
    {
        return eat();
    }
    return ASTToken_t::none;
}

ASTToken ASTTokenRibbon::eat()
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "TokenRibbon", "Eat token (idx %i) %s \n", m_cursor, peek().string().c_str() );
    return m_tokens.at(m_cursor++);
}

void ASTTokenRibbon::start_transaction()
{
    m_transaction.push(m_cursor);
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "TokenRibbon", "Start Transaction (idx %i)\n", m_cursor);
}

void ASTTokenRibbon::rollback()
{
    m_cursor = m_transaction.top();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "TokenRibbon", "Rollback (idx %i)\n", m_cursor);
    m_transaction.pop();
}

void ASTTokenRibbon::commit()
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "TokenRibbon", "Commit (idx %i)\n", m_cursor);
    m_transaction.pop();
}

void ASTTokenRibbon::reset(const char* new_buffer, size_t new_size)
{
    auto buffer = const_cast<char*>(new_buffer);

    m_tokens.clear();

    m_global_token.set_external_buffer( buffer, 0, new_size, true ); // wraps all

    while(!m_transaction.empty())
        m_transaction.pop();

    m_cursor = 0;
}

bool ASTTokenRibbon::can_eat(size_t count) const
{
    ASSERT(count > 0);
    return m_cursor + count <= m_tokens.size() ;
}

std::string ASTTokenRibbon::range_to_string(size_t pos, int size)
{
    ASSERT( size != 0);
    ASSERT( pos < m_tokens.size() );

    // ensure size is positive
    if( size < 0 )
    {
        ASSERT( -size < m_tokens.size() - pos );
        pos  = pos + size;
        size = -size;
    }
    else
    {
        ASSERT( pos + size < m_tokens.size() );
    }

    std::string result;
    for( size_t i = pos; i < pos + size; ++i )
        result = result + m_tokens[pos].string();
    return result;
}

#pragma once

#include <string>
#include <vector>
#include <stack>

#include "ASTToken.h"

namespace ndbl
{
    /**
     * This class wraps a container to store a list of Token.
     * An internal cursor points to a current token, cursor can be moved by calling eat or eat_if.
     * A transaction system allows to commit or rollback a sequence of eating.
     */
    class ASTTokenRibbon
    {
    public:
        ASTTokenRibbon()
        : m_cursor(0)
        , m_global_token(ASTToken_t::ignore)
        {}

        void                reset(const char* buffer = nullptr, size_t size = 0);
        ASTToken&           at(size_t index) { return m_tokens.at(index); }
        ASTToken&           back() { return m_tokens.back(); };
        std::vector<ASTToken>::iterator
                            begin() { return m_tokens.begin(); };
        std::vector<ASTToken>::iterator
                            end() { return m_tokens.end(); };
        bool                can_eat(size_t count = 1)const;
        std::string         range_to_string(size_t begin, size_t end); // format ribbon from range [begin, end-1]
        ASTToken            eat();           // Return the next token and increment cursor
        ASTToken            eat_if(ASTToken_t); // Only if next token has a given type: returns it and increment cursor
        bool                empty()const { return m_tokens.empty(); }
        const ASTToken&     get_eaten()const { ASSERT(m_cursor > 0); return m_tokens[m_cursor - 1];}
        bool                peek(ASTToken_t t)const { return m_tokens[m_cursor].m_type == t; }
        const ASTToken&     peek()const { return m_tokens[m_cursor]; }
        ASTToken&           push(ASTToken&);
        ASTToken&           global_token() { return m_global_token; }
        size_t              size()const { return m_tokens.size(); }
        std::string         to_string() const; // Generate a colored string highlighting the current and past tokens
        void                start_transaction();    // Start a transaction by saving the cursor position in a stack (allows nested transactions).
        void                rollback(); // Restore the cursor position where the last transaction started.
        void                commit();   // Commit the current transaction.

    private:
        size_t              m_cursor; // current token index
        ASTToken            m_global_token; // wraps the whole buffer
        std::vector<ASTToken>  m_tokens;
        std::stack<size_t>  m_transaction; // transaction start indexes
    };
}

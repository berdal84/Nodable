#pragma once
#include "tools/core/types.h"

namespace ndbl {

    /**
     * @enum Identifies each Type of Token that a Language should state.
     * @note When the parser find a match, it assign a given Token_t to the parsed token.
     * @example
     *     "bool" => Token_t::keyword_bool
     *     "100"  => Token_t::literal_int
     */
	enum class ASTToken_t: i8_t
	{
        none = 0, // to say 'absence of token'

        ignore,
        keyword_if,
        keyword_else,
        keyword_for,
        keyword_while,
        keyword_operator,
        //----- types -------
        keyword_string,
        keyword_double,
        keyword_int,
        keyword_i16,
        keyword_bool,
        keyword_any,     // like TypeScript's
        keyword_unknown, // like TypeScript's
        //----- literals -----
        literal_string,
        literal_double,
        literal_int,
        literal_bool,
        literal_any,
        literal_unknown,
        operator_,
        identifier,
        parenthesis_open,
        parenthesis_close,
        list_separator,
        scope_begin,
        scope_end ,
        end_of_instruction,
        end_of_line,
    };

    /** Check if a given keyword is a type (ex: bool, int, double,...)*/
	static constexpr bool is_a_type_keyword(ASTToken_t _token_t)
    {
        return ASTToken_t::keyword_string <= _token_t && _token_t <= ASTToken_t::keyword_unknown;
    }

}


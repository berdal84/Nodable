#include "Dictionnary.h"

using namespace Nodable;

std::string Dictionnary::convert(const TokenType& _type)const
{
    return tokenTypeToString.at(_type);
}

void Dictionnary::insert(
    std::regex _regex,
    TokenType _tokenType)
{    
    NODABLE_ASSERT( tokenTypeToRegex.find(_tokenType) == tokenTypeToRegex.end() ); // Unable to add another regex, this token alread has one.

    tokenTypeToRegex[_tokenType] = _regex;
}

void Dictionnary::insert(
    std::string _string,
    TokenType _tokenType)
{
    tokenTypeToString[_tokenType] = _string;

    // Clean string before to create a regex
    // TODO: improve it to solve all regex escape char problems
    if (_string.size() == 1) {
        _string.insert(_string.begin(), '[');
        _string.insert(_string.end()  , ']');
    }

    insert(std::regex("^" + _string), _tokenType);
    
}

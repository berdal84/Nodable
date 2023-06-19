#include <fw/core/string.h>
#include <cstring>

using namespace fw;

std::string string::fmt_double(double d)
{
    std::string str {std::to_string(d)};
    limit_trailing_zeros(str, 1);
    return str;
}

std::string string::fmt_hex(u64_t _addr)
{
    char str[33];
    snprintf(str, sizeof(str), "%#llx", _addr);
    return {str};
}

std::string string::fmt_ptr(const void* _addr)
{
    char str[33];
    snprintf(str, sizeof(str), "%p", _addr);
    return {str};
}

void string::limit_trailing_zeros(std::string& str, int _trailing_max)
{
    // limit to _trailing_max zeros
    size_t first_zero_to_remove = str.find_last_not_of('0', str.find_last_of('.')) + 1 + _trailing_max;
    str.erase( std::min( first_zero_to_remove , std::string::npos), std::string::npos);

    // erase eventual dot
    if ( _trailing_max == 0 && str.find_last_of('.') + 1 == str.size())
    {
        str.erase(str.find_last_of('.'), std::string::npos);
    }

    if( str.back() == '.')
    {
        str.push_back('0');
    }

}

std::string string::fmt_title(const char *_title, int _width)
{
    /*
     * Takes _title and do:
     * ------------<=[ _title ]=>------------
     */

    const char* pre       = "-=[ ";
    const char* post      = " ]=-";
    const char* padding   = "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
                            "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-";

    int pad_size = (_width - strlen(_title) - strlen(pre) - strlen(post)) / 2;

    char result[256];
    snprintf(result, _width, "%*.*s%s%s%s%*.*s\n",
             0, pad_size, padding,
             pre, _title, post,
             0, pad_size-1, padding
             );
    return result;
}

